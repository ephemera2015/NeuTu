#include "dvidbranchdialog.h"
#include "ui_dvidbranchdialog.h"

#include <iostream>
#include <stdlib.h>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QModelIndex>
#include <QStandardItem>
#include <QStringList>
#include <QStringListModel>
#include <QsLog.h>

#include "dvid/zdvidtarget.h"
#include "dvid/zdvidnode.h"
#include "dialogs/zdviddialog.h"
#include "neutubeconfig.h"

DvidBranchDialog::DvidBranchDialog(QWidget *parent) :
    ZDvidTargetProviderDialog(parent),
    ui(new Ui::DvidBranchDialog)
{
    ui->setupUi(this);

    m_reader.setVerbose(false);

    // UI configuration
    ui->todoBox->setPlaceholderText("default");
    ui->bodyLabelBox->setPlaceholderText("default");
    ui->ROIBox->setPlaceholderText("none");

    // UI connections
    connect(ui->repoListView, SIGNAL(clicked(QModelIndex)), this, SLOT(onRepoClicked(QModelIndex)));
    connect(ui->branchListView, SIGNAL(clicked(QModelIndex)), this, SLOT(onBranchClicked(QModelIndex)));
    connect(ui->detailsButton, SIGNAL(clicked(bool)), this, SLOT(toggleDetailsPanel()));
    connect(ui->oldDialogButton, SIGNAL(clicked(bool)), this, SLOT(launchOldDialog()));

    // models & related
    m_repoModel = new QStringListModel();
    ui->repoListView->setModel(m_repoModel);

    m_branchModel = new QStringListModel();
    ui->branchListView->setModel(m_branchModel);

    // populate first panel with server data
    hideDetailsPanel();
    loadDatasets();

}

// constants
const QString DvidBranchDialog::KEY_REPOS = "repos";
const QString DvidBranchDialog::KEY_NAME = "name";
const QString DvidBranchDialog::KEY_SERVER = "server";
const QString DvidBranchDialog::KEY_PORT = "port";
const QString DvidBranchDialog::KEY_UUID = "UUID";
const QString DvidBranchDialog::KEY_DESCRIPTION = "description";
const QString DvidBranchDialog::KEY_DAG = "DAG";
const QString DvidBranchDialog::KEY_NODES = "Nodes";
const QString DvidBranchDialog::KEY_NOTE = "Note";

const int DvidBranchDialog::DEFAULT_PORT = 8000;

const QString DvidBranchDialog::DEFAULT_MASTER_NAME = "master";
const QString DvidBranchDialog::MESSAGE_LOADING = "Loading...";

/*
 * load stored dataset list into the first panel of the UI
 */
void DvidBranchDialog::loadDatasets() {
    // also functions as a reset
    clearNode();

    // for now, load from file; probably should be from
    //  DVID, some other db, or a Fly EM service of some kind
    QJsonObject jsonData = loadDatasetsFromFile();
    if (jsonData.isEmpty()) {
        // need some message in UI?
        return;
    }
    if (!jsonData.contains(KEY_REPOS)) {
        // message?
        return;
    }

    // we'll keep the repo data as json, but index it a bit
    m_repoMap.clear();
    foreach(QJsonValue value, jsonData[KEY_REPOS].toArray()) {
        QJsonObject repo = value.toObject();
        m_repoMap[repo[KEY_NAME].toString()] = repo;
    }

    // note that once this is populated, it's never changed or cleared
    QStringList repoNameList = m_repoMap.keys();
    repoNameList.sort();
    m_repoModel->setStringList(repoNameList);
}

/*
 * load stored dataset from file into json
 */
QJsonObject DvidBranchDialog::loadDatasetsFromFile() {
    QJsonObject empty;

    // testing: hard-coded file location
    QString filepath = "/Users/olbrisd/projects/flyem/NeuTu/testing/test-repos.json";
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        showError("Error loading datasets", "Couldn't open dataset file " + filepath + "!");
        return empty;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() or !doc.isObject()) {
        showError("Error parsing file", "Couldn't parse file " + filepath + "!");
        return empty;
    } else {
        LINFO() << "Read DVID repo information json from file" + filepath;
        return doc.object();
    }
}

void DvidBranchDialog::onRepoClicked(QModelIndex modelIndex) {
    clearNode();
    loadBranches(m_repoModel->data(modelIndex, Qt::DisplayRole).toString());
}

/*
 * given a repo name, load all its branches into the UI
 */
void DvidBranchDialog::loadBranches(QString repoName) {
    LINFO() << "Loading branches from DVID repo" << repoName.toStdString();

    m_repoName = repoName;

    // clear model; use it as a loading message, too
    QStringList branchNames;
    branchNames.append(MESSAGE_LOADING);
    m_branchModel->setStringList(branchNames);


    // read repo info, in this thread; there's nothing else the user can do
    //  but wait at this point anyway
    ZDvidTarget target(m_repoMap[m_repoName][KEY_SERVER].toString().toStdString(),
        m_repoMap[m_repoName][KEY_UUID].toString().toStdString(),
        m_repoMap[m_repoName][KEY_PORT].toInt());
    if (!m_reader.open(target)) {
        QMessageBox errorBox;
        errorBox.setText("Error connecting to DVID");
        errorBox.setInformativeText("Could not conenct to DVID at " +
            QString::fromStdString(target.getAddressWithPort()) +
            "!  Check server information!");
        errorBox.setStandardButtons(QMessageBox::Ok);
        errorBox.setIcon(QMessageBox::Warning);
        errorBox.exec();
        branchNames.clear();
        m_branchModel->setStringList(branchNames);
        return;
    }
    ZJsonObject info = m_reader.readInfo();
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(info.dumpString()).toUtf8());
    QJsonObject repoJson = doc.object();

    // parse out actual branches and populate the model
    QJsonObject nodeJson = repoJson[KEY_DAG].toObject()[KEY_NODES].toObject();
    QStringList branchUUIDs = findBranches(nodeJson);

    branchNames.clear();
    foreach (QString uuid, branchUUIDs) {
        QJsonObject branchNode = nodeJson[uuid].toObject();
        QString branchName = branchNode["Branch"].toString();
        m_branchMap[branchName] = branchNode;
        branchNames.append(branchName);
    }

    // at this point we have to deal with the branch name "" (the empty string);
    //  that is to be interpreted as being the master branch,
    //  so name it explicitly in the UI (top of list); first,
    //  check that no other branch is already using the name we want to use
    if (branchNames.contains("")) {
        QString masterName = DEFAULT_MASTER_NAME;
        if (branchNames.contains(DEFAULT_MASTER_NAME)) {
            masterName = findMasterName(DEFAULT_MASTER_NAME, branchNames);
        }

        // now rename in local data structures (not in DVID)
        branchNames.removeAll("");
        branchNames.prepend(masterName);

        m_branchMap[masterName] = m_branchMap[""];
        m_branchMap.remove("");
    }

    m_branchModel->setStringList(branchNames);
}

/*
 * find a branch name not in the input list
 */
QString DvidBranchDialog::findMasterName(QString prefix, QStringList names) {
    int n = 2;
    QString temp = QString("%1 %2").arg(prefix, QString::number(n));
    while (names.contains(temp)) {
        n++;
        temp = QString("%1 %2").arg(prefix, QString::number(n));
    }
    return temp;
}

/*
 * given the DVID-returned node json for a repo, return a list
 * of the UUID keys into that json that correspond to the nodes
 * that are tips of branches
 */
QStringList DvidBranchDialog::findBranches(QJsonObject nodeJson) {

    QStringList UUIDs;

    // branches should have nodes in a parent-child relationship;
    //  the node with a given branch name that has no children with
    //  the branch name is the tip (since it can have children with
    //  different branch names)

    // problem is that we have to be able to handle old repos that don't
    //  enforce the branch convention; eg, tons of unnamed branches that
    //  have no relation to each other

    // first pass: separate by branch name; accumulate parent versions

    // branch name: list of uuids
    QMap<QString, QStringList> branches;
    // branch name: set of parent version IDs
    QMap<QString, QSet<int>> parents;

    foreach (QString uuid, nodeJson.keys()) {
        QJsonObject node = nodeJson[uuid].toObject();
        QString branchName = node["Branch"].toString();
        branches[branchName].append(uuid);
        foreach (QJsonValue parent, node["Parents"].toArray()) {
            parents[branchName].insert(parent.toInt());
        }
    }

    // second pass: for each branch, find the tip; if a node isn't the
    //  parent of any other node in the branch, it's a tip; if there's more than
    //  one, that's a result of older repos that don't respect the rules
    //  we have no way to pick a commit, so eliminate it from the list (for now)
    foreach (QString branchName, branches.keys()) {
        QList<QString> tiplist;
        foreach (QString uuid, branches[branchName]) {
            QJsonObject node = nodeJson[uuid].toObject();
            if (!parents[branchName].contains(node["VersionID"].toInt())) {
                tiplist.append(uuid);
            }
        }
        if (tiplist.size() != 1) {
            LINFO() << "Branch" << branchName.toStdString() << "has indeterminate tip";

            // not sure if I need a dialog here, but it's useful for testing
            showError("Branch error", "Can't find the tip of branch '" + branchName + "'; it won't be listed.");
        } else {
            UUIDs.append(tiplist[0]);
        }
    }
    return UUIDs;
}

void DvidBranchDialog::onBranchClicked(QModelIndex modelIndex) {
    loadNode(m_branchModel->data(modelIndex, Qt::DisplayRole).toString());
}

/*
 * given branch name, load the data for the node into the third column of the UI
 */
void DvidBranchDialog::loadNode(QString branchName) {
    // this is kind of ad hoc: filter out the loading message
    if (branchName == MESSAGE_LOADING) {
        return;
    }

    m_branchName = branchName;

    // server and port are from repo:
    ui->serverBox->setText(m_repoMap[m_repoName][KEY_SERVER].toString());
    ui->portBox->setValue(m_repoMap[m_repoName][KEY_PORT].toInt());

    // rest is from the specific node:
    QJsonObject nodeJson = m_branchMap[m_branchName];
    ui->UUIDBox->setText(nodeJson[KEY_UUID].toString().left(4));
    ui->commentBox->setText(nodeJson[KEY_NOTE].toString());
    ui->commentBox->setCursorPosition(0);
    ui->commentBox->setToolTip(nodeJson[KEY_NOTE].toString());

    // check for default settings
    ZJsonObject defaultsJson;
    defaultsJson = m_reader.readDefaultDataSetting();
    QJsonDocument doc = QJsonDocument::fromJson(QString::fromStdString(defaultsJson.dumpString()).toUtf8());
    QJsonObject defaults = doc.object();

    // should make these keys constants?
    if (defaults.contains("segmentation")) {
        ui->labelsBox->setText(defaults["segmentation"].toString());
    }
    if (defaults.contains("grayscale")) {
        ui->grayscaleBox->setText(defaults["grayscale"].toString());
    }
    if (defaults.contains("synapses")) {
        ui->synapsesBox->setText(defaults["synapses"].toString());
    }
    if (defaults.contains("todos")) {
        ui->todoBox->setText(defaults["todos"].toString());
    }
    if (defaults.contains("bodies")) {
        ui->bodyLabelBox->setText(defaults["bodies"].toString());
    }

#if defined(_FLYEM_)
    ui->librarianCheckBox->setChecked(true);
    ui->librarianBox->setText(QString::fromStdString(GET_FLYEM_CONFIG.getDefaultLibrarian()));
#endif

}

ZDvidTarget &DvidBranchDialog::getDvidTarget() {

    m_dvidTarget.clear();


    // hard-coded test
    // m_dvidTarget.setServer("emdata3.int.janelia.org");
    // m_dvidTarget.setUuid("100e7");
    // m_dvidTarget.setPort(8000);

    m_dvidTarget.setServer(ui->serverBox->text().toStdString());
    m_dvidTarget.setUuid(ui->UUIDBox->text().toStdString());
    m_dvidTarget.setPort(ui->portBox->text().toInt());

    m_dvidTarget.setComment(ui->commentBox->text().toStdString());

    m_dvidTarget.setLabelBlockName(ui->labelsBox->text().toStdString());
    m_dvidTarget.setGrayScaleName(ui->grayscaleBox->text().toStdString());
    m_dvidTarget.setSynapseName(ui->synapsesBox->text().toStdString());

    m_dvidTarget.setMultiscale2dName(ui->tilesBox->text().toStdString());
    m_dvidTarget.configTile(ui->tilesBox->text().toStdString(), ui->tilesCheckBox->isChecked());
    m_dvidTarget.setRoiName(ui->ROIBox->text().toStdString());

    m_dvidTarget.setTodoListName(ui->todoBox->text().toStdString());
    m_dvidTarget.setBodyLabelName(ui->bodyLabelBox->text().toStdString());

    if (!ui->grayscaleSourceCheckBox->isChecked()) {
        ZDvidNode grayscaleNode;
        grayscaleNode.setServer(ui->grayscaleServerBox->text().toStdString());
        grayscaleNode.setPort(ui->grayscalePortBox->text().toInt());
        grayscaleNode.setUuid(ui->grayscaleUUIDBox->text().toStdString());
        m_dvidTarget.setGrayScaleSource(grayscaleNode);
    }

    if (!ui->tileSourceCheckBox->isChecked()) {
        ZDvidNode tileNode;
        tileNode.setServer(ui->tileServerBox->text().toStdString());
        tileNode.setPort(ui->tilePortBox->text().toInt());
        tileNode.setUuid(ui->tileUUIDBox->text().toStdString());
        m_dvidTarget.setTileSource(tileNode);
    }

    if (ui->librarianCheckBox->isChecked()) {
        m_dvidTarget.setSupervisorServer(ui->librarianBox->text().toStdString());
    }

    if (ui->readOnlyCheckBox->isChecked()) {
        m_dvidTarget.setReadOnly(true);
    }

    return m_dvidTarget;
}

/*
 * clear the info in the third column of the UI
 */
void DvidBranchDialog::clearNode() {
    ui->serverBox->clear();
    ui->portBox->setValue(DEFAULT_PORT);
    ui->UUIDBox->clear();

    ui->commentBox->clear();

    ui->labelsBox->clear();
    ui->grayscaleBox->clear();
    ui->synapsesBox->clear();

    ui->ROIBox->clear();
    ui->tilesBox->clear();
    ui->tilesCheckBox->setChecked(false);
    ui->todoBox->clear();
    ui->bodyLabelBox->clear();

    ui->grayscaleSourceCheckBox->setChecked(true);
    ui->grayscaleServerBox->clear();
    ui->grayscalePortBox->setValue(DEFAULT_PORT);
    ui->grayscaleUUIDBox->clear();

    ui->tileSourceCheckBox->setChecked(true);
    ui->tileServerBox->clear();
    ui->tilePortBox->setValue(DEFAULT_PORT);
    ui->tileUUIDBox->clear();

    ui->librarianBox->clear();
    ui->librarianCheckBox->setChecked(false);
}

/*
 * launch the old dialog and forward its value to this dialog
 */
void DvidBranchDialog::launchOldDialog() {

    // pop up the old dialog and transfer values to the UI, if user clicks OK

    ZDvidDialog * dialog = new ZDvidDialog(NULL);
    if (dialog->exec()) {
        clearNode();
        ZDvidTarget target = dialog->getDvidTarget();

        ui->serverBox->setText(QString::fromStdString(target.getAddress()));
        ui->portBox->setValue(target.getPort());
        ui->UUIDBox->setText(QString::fromStdString(target.getUuid()));

        ui->commentBox->setText(QString::fromStdString(target.getComment()));

        ui->labelsBox->setText(QString::fromStdString(target.getLabelBlockName()));
        ui->grayscaleBox->setText(QString::fromStdString(target.getGrayScaleName()));
        ui->synapsesBox->setText(QString::fromStdString(target.getSynapseName()));

        ui->ROIBox->setText(QString::fromStdString(target.getRoiName()));
        ui->tilesBox->setText(QString::fromStdString(target.getMultiscale2dName()));
        ui->tilesCheckBox->setChecked(target.isLowQualityTile(target.getMultiscale2dName()));
        ui->todoBox->setText(QString::fromStdString(target.getTodoListName()));
        ui->bodyLabelBox->setText(QString::fromStdString(target.getBodyLabelName()));

        if (target.getGrayScaleSource().isValid()) {
            ui->grayscaleSourceCheckBox->setChecked(true);
            ui->grayscaleServerBox->setText(QString::fromStdString(target.getGrayScaleSource().getAddress()));
            ui->grayscalePortBox->setValue(target.getGrayScaleSource().getPort());
            ui->grayscaleUUIDBox->setText(QString::fromStdString(target.getGrayScaleSource().getUuid()));
        }

        if (target.getTileSource().isValid()) {
            ui->tileSourceCheckBox->setChecked(true);
            ui->tileServerBox->setText(QString::fromStdString(target.getTileSource().getAddress()));
            ui->tilePortBox->setValue(target.getTileSource().getPort());
            ui->tileUUIDBox->setText(QString::fromStdString(target.getTileSource().getUuid()));
        }

        if (target.isSupervised()) {
            ui->librarianCheckBox->setChecked(true);
            ui->librarianBox->setText(QString::fromStdString(target.getSupervisor()));
        }

    }
    delete dialog;

}

/*
 * toggle the details panel open and closed
 */
void DvidBranchDialog::toggleDetailsPanel() {
    if (m_detailsVisible) {
        hideDetailsPanel();
    } else {
        showDetailsPanel();
    }
}

void DvidBranchDialog::showDetailsPanel() {
    // here and in hide, below, I want the dialog width to adjust
    //  for adding and removing a third panel of same width as the other two
    resize(3 * width() / 2, height());
    ui->detailsWidget->show();
    ui->detailsButton->setText("<< Details");
    m_detailsVisible = true;
}

void DvidBranchDialog::hideDetailsPanel() {
    resize(2 * width() / 3, height());
    ui->detailsWidget->hide();
    ui->detailsButton->setText("Details >>");
    m_detailsVisible = false;
}

/*
 * input: title and message for error dialog
 * effect: shows error dialog (convenience function)
 */
void DvidBranchDialog::showError(QString title, QString message) {
    QMessageBox errorBox;
    errorBox.setText(title);
    errorBox.setInformativeText(message);
    errorBox.setStandardButtons(QMessageBox::Ok);
    errorBox.setIcon(QMessageBox::Warning);
    errorBox.exec();
}

DvidBranchDialog::~DvidBranchDialog()
{
    delete ui;
}
