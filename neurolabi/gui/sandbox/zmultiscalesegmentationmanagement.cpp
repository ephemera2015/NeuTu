#include <queue>
#include <QStandardItem>
#include <QPushButton>
#include <QLabel>
#include <QAction>
#include <QString>
#include <QLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <set>
#include "time.h"
#include "zmultiscalesegmentationmanagement.h"
#include "zstackobject.h"
#include "flyem/zstackwatershedcontainer.h"
#include "zobject3dscanarray.h"
#include "zstackframe.h"
#include "zsandbox.h"
#include "zstackdocdatabuffer.h"
#include "imgproc/zstackprocessor.h"
#include "imgproc/zwatershedmst.h"
#include "zstackdoc.h"
#include "segment/zsegmentationnodewrapper.h"
#include "mainwindow.h"


using std::queue;


void onSelectMask(ZStackObject* obj)
{
  string id = obj->getObjectId();
  ZMultiscaleSegmentationWindow::instance()->selectNode(id);
}


void onDeselectMask(ZStackObject* obj)
{
  string id = obj->getObjectId();
  ZMultiscaleSegmentationWindow::instance()->deselectNode(id);
}


void ZMultiscaleSegmentationWindow::update(const ZSegmentationTree* /*tree*/, const std::string &id){
  updateMask(id);
  updateTreeView(id);
}


void ZMultiscaleSegmentationWindow::updateMask(const std::string &id){
  if(!m_frame){
    return;
  }

  //remove existing masks
  m_frame->document()->removeObject(ZStackObjectRole::ROLE_SEGMENTATION,true);

  ZColorScheme scheme;
  scheme.setColorScheme(ZColorScheme::UNIQUE_COLOR);
  int i = 1;

  queue<string> ids;
  if(m_seg_tree->isLeaf(id)){
    ids.push(id);
  } else {
    if(m_show_leaves->isChecked()){
      for(auto t: m_seg_tree->getLeavesIDs(id)){
        ids.push(t);
      }
    } else {
      for(auto t: m_seg_tree->getChildrenIDs(id)){
        ids.push(t);
      }
    }
  }

  while(!ids.empty()){
    string t = ids.front();
    ids.pop();
    ZSegmentationNodeWrapper* wrapper = new ZSegmentationNodeWrapper(m_seg_tree,t);
    wrapper->setColor(scheme.getColor(i++));
    wrapper->setObjectId(t);
    wrapper->addCallBackOnSelection(onSelectMask);
    wrapper->addCallBackOnDeselection(onDeselectMask);
    wrapper->addRole(ZStackObjectRole::ROLE_SEGMENTATION);
    wrapper->setSelectable(true);
    if(m_show_contour->isChecked()){
      wrapper->setDisplayStyle(ZStackObject::BOUNDARY);
    } else {
      wrapper->setDisplayStyle(ZStackObject::SOLID);
    }
    m_frame->document()->getDataBuffer()->addUpdate(wrapper, ZStackDocObjectUpdate::ACTION_ADD_NONUNIQUE);
  }
}


void ZMultiscaleSegmentationWindow::onNodeItemClicked(QTreeWidgetItem *item, int){
  m_selected_id = item->text(0).toStdString();
  updateMask(m_selected_id);
}


void ZMultiscaleSegmentationWindow::selectNode(const string& id){
  m_selected_id = id;
  for(QTreeWidgetItem* wgt:m_view->selectedItems()){
    wgt->setSelected(false);
  }

  QList<QTreeWidgetItem*> items = m_view->findItems(QString::fromStdString(id),Qt::MatchExactly | Qt::MatchRecursive);
  if(items.size() > 0){
    QTreeWidgetItem* item = items.at(0);
    item->setSelected(true);
  }
}


void ZMultiscaleSegmentationWindow::deselectNode(const string& id){
  m_selected_id = m_seg_tree->getRootID();
  QList<QTreeWidgetItem*> items = m_view->findItems(QString::fromStdString(id),Qt::MatchExactly | Qt::MatchRecursive);
  if(items.size() > 0){
    QTreeWidgetItem* item = items.at(0);
    item->setSelected(false);
  }
}


void ZMultiscaleSegmentationWindow::updateTreeView(const std::string &id){
  QList<QTreeWidgetItem*> items = m_view->findItems(QString::fromStdString(id),Qt::MatchExactly | Qt::MatchRecursive);
  if(items.size() > 0){
    QTreeWidgetItem* item = items.at(0);
    while(item->childCount() > 0){
      item->removeChild(item->child(0));
    }
  }

  queue<string> que;
  que.push(id);
  while(!que.empty()){
    string t = que.front();
    que.pop();

    items = m_view->findItems(QString::fromStdString(t),Qt::MatchExactly | Qt::MatchRecursive);
    if(items.size() > 0){
      QTreeWidgetItem* item = items.at(0);
      vector<string> ids = m_seg_tree->getChildrenIDs(t);
      for(auto child_id : ids){
        QTreeWidgetItem* child = new QTreeWidgetItem(QStringList(QString::fromStdString(child_id)));
        item->addChild(child);
        if(!m_seg_tree->isLeaf(child_id)){
          que.push(child_id);
        }
      }
      item->setExpanded(true);
    }
  }
}


ZMultiscaleSegmentationWindow* ZMultiscaleSegmentationWindow::s_window = nullptr;


ZMultiscaleSegmentationWindow::ZMultiscaleSegmentationWindow()
{
  init();
}


void ZMultiscaleSegmentationWindow::init()
{
  m_frame = NULL;
  m_stack = NULL;
  //m_super_voxels = shared_ptr<ZSuperVoxelManager>(new ZSuperVoxelManager());

  this->setWindowTitle("Multiscale Segmentation Project");
  Qt::WindowFlags flags = this->windowFlags();
  flags |= Qt::WindowStaysOnTopHint;
  this->setWindowFlags(flags);

  m_view = new QTreeWidget();
  m_view->setHeaderLabel("Segmentation Tree");

  QPushButton* open_stack = new QPushButton("Open");
  QPushButton* segment = new QPushButton("Segment");
  QPushButton* merge = new QPushButton("Merge");
  QPushButton* _export = new QPushButton("Export");
  //QPushButton* create_super_voxels = new QPushButton("Create Super Voxels");
  m_show_leaves = new QCheckBox("Show Leaves");
  m_enable_super_voxel = new QCheckBox("Super Voxel");
  m_show_contour =new QCheckBox("Show Contour");

  //m_merge_from = new QLineEdit();
  //m_merge_to = new QLineEdit();

  //setup layout
  QGridLayout* lay=new QGridLayout(this);
  lay->addWidget(m_view,0,0,8,16);

  lay->addWidget(m_enable_super_voxel,9,0,1,4);
  lay->addWidget(m_show_contour,9,4,1,4);
  lay->addWidget(m_show_leaves,9,8,1,4);

  lay->addWidget(open_stack,10,0,1,4);
  lay->addWidget(segment,10,4,1,4);
  lay->addWidget(merge,10,8,1,4);
  lay->addWidget(_export,10,12,1,4);
  //lay->addWidget(create_super_voxels,11,0,1,5);

  /*lay->addWidget(new QLabel("From:"),11,0,1,2);
  lay->addWidget(m_merge_from,11,2,1,3);
  lay->addWidget(new QLabel("To:"),11,5,1,1);
  lay->addWidget(m_merge_to,11,6,1,3);*/


  connect(open_stack,SIGNAL(clicked()),this,SLOT(onOpenStack()));
  connect(segment,SIGNAL(clicked()),this,SLOT(onSegment()));
  connect(merge,SIGNAL(clicked()),this,SLOT(onMerge()));
  connect(_export,SIGNAL(clicked()),this,SLOT(onExport()));
  connect(m_view,SIGNAL(itemClicked(QTreeWidgetItem*,int)),this,SLOT(onNodeItemClicked(QTreeWidgetItem*,int)));
  connect(m_show_leaves,SIGNAL(stateChanged(int)),this,SLOT(onShowLeaves(int)));
  //connect(create_super_voxels,SIGNAL(clicked()),this,SLOT(onCreateSuperVoxels()));

  this->setLayout(lay);
  this->setMinimumSize(400,600);
  this->move(300,200);
}


ZMultiscaleSegmentationWindow::~ZMultiscaleSegmentationWindow(){

}

/*
void ZMultiscaleSegmentationWindow::onCreateSuperVoxels(){
  if(!m_frame || ! m_stack){
    return;
  }
  uint start_label = 1;

  std::cout<<"Begin Creating Super Voxels..."<<std::endl;
  auto super_voxel_stack = createSuperVoxel(*m_stack,start_label);
  std::cout<<"Creating Super Voxels Done"<<std::endl;

  if(super_voxel_stack){
    m_super_voxels ->fromStack(*super_voxel_stack,*m_stack);
  }
}*/


void ZMultiscaleSegmentationWindow::onExport(){
  if(!m_stack){
    return;
  }
  int label = 1;
  shared_ptr<ZStack> segmentation = shared_ptr<ZStack>(m_stack->clone());
  segmentation->setZero();
  for(const string& id: m_seg_tree->getLeavesIDs(m_seg_tree->getRootID())){
    m_seg_tree->labelStack(id,segmentation.get(),label++);
  }
  QString file_name = QFileDialog::getSaveFileName(this,"Save Segmentation","","tiff(*.tif)");
  if(file_name != ""){
    segmentation->save(file_name.toStdString());
  }
}


void ZMultiscaleSegmentationWindow::onShowLeaves(int state){
  if(state == Qt::Checked){
    updateMask(m_selected_id);
  }
}


void ZMultiscaleSegmentationWindow::onMerge(){
  if(!m_frame){
    return;
  }
  QList<ZSegmentationNodeWrapper*> list = m_frame->document()->getSelectedObjectList<ZSegmentationNodeWrapper>(ZStackObject::TYPE_SEGMENTATION_ENCODER);
  string to = m_selected_id;
  for(auto it = list.begin(); it != list.end(); ++it){
    string from = (*it)->getNodeID();
    if(from != to){
      m_seg_tree->merge(from,to);
    }
  }
}


ZStack* ZMultiscaleSegmentationWindow::makeSelectedStack(){
  ZStack* rv = nullptr;
  if(m_selected_id == m_seg_tree->getRootID()){
    rv = m_stack->clone();
  } else {
    rv = m_stack->makeCrop(m_seg_tree->getBoundBox(m_selected_id));
    m_seg_tree->maskStack(m_selected_id, rv);
  }
  return rv;
}


void ZMultiscaleSegmentationWindow::onSegment(){
  //test();
  if(!m_frame || !m_stack){
    return;
  }

  if(m_enable_super_voxel->isChecked()){
    onSuperVoxel();
  } else {
    shared_ptr<ZStack> stack = shared_ptr<ZStack>(makeSelectedStack());
    if(!stack){
      return ;
    }
    ZStackWatershedContainer container(stack.get());
    container.setDsMethod("Min(ignore zero)");
    std::vector<ZStackObject*> seeds = getSeeds();
    for(ZStackObject* seed: seeds){
      container.addSeed(seed);
    }
    if(stack->getVoxelNumber() > 1e7){
      container.setScale(2);
      std::cout<<"Scale: "<<2<<std::endl;
    }
    container.run();
    ZStackPtr presult = container.getResultStack();
    if(presult){
      clock_t start = std::clock();
      m_seg_tree->consume(m_selected_id,presult.get());//here
      clock_t end = std::clock();
      std::cout<<"Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<"ms"<<std::endl;
      std::cout<<"Mem Usage: "<<m_seg_tree->memUsage()/1024.0/1024.0<<"M"<<std::endl;
    }
    removeSeeds();
  }
}


void ZMultiscaleSegmentationWindow::onSuperVoxel(){
  shared_ptr<ZStack> stack = shared_ptr<ZStack>(makeSelectedStack());
  if(!stack){
    return;
  }

  ZMSTContainer container;

  //clock_t start =  clock();
  shared_ptr<ZStack> rv = container.run(*stack, getSeeds());
  //clock_t end = clock();


  if(rv){
    m_seg_tree->consume(m_selected_id,*rv);
  }
  removeSeeds();
}


void ZMultiscaleSegmentationWindow::onOpenStack(){
  QString file_name = QFileDialog::getOpenFileName(this,"Open Stack","","tiff(*.tif)");
  if (file_name!=""){
    if(m_frame){
      m_frame->close();
      m_frame = NULL;
    }
    m_stack = new ZStack();
    m_stack->load(file_name.toStdString());
    m_frame = ZSandbox::GetMainWindow()->createStackFrame(m_stack);
    ZSandbox::GetMainWindow()->addStackFrame(m_frame);
    ZSandbox::GetMainWindow()->presentStackFrame(m_frame);

    m_seg_tree = std::shared_ptr<ZSegmentationTree>(new ZSegmentationTree());
    m_selected_id = m_seg_tree->getRootID();
    m_seg_tree->addObserver(this);
    m_view->clear();
    m_view->insertTopLevelItem(0,new QTreeWidgetItem(QStringList(QString::fromStdString(m_seg_tree->getRootID()))));
  }
}


void ZMultiscaleSegmentationWindow::removeSeeds(){
  if(!m_frame){
    return;
  }
  for(ZStroke2d* stroke:m_frame->document()->getStrokeList()){
    m_frame->document()->removeObject(stroke);
  }
}


std::vector<ZStackObject*> ZMultiscaleSegmentationWindow::getSeeds(){
  if(!m_frame){
    return std::vector<ZStackObject*>();
  }

  std::vector<ZStackObject*> seeds;
  std::map<QString,int> color_indices;
  int i = 0;

  for(ZStroke2d* stroke:m_frame->document()->getStrokeList()){
    std::map<QString,int>::iterator p_index = color_indices.find(stroke->getColor().name());
    if (p_index==color_indices.end()){
      color_indices.insert(std::make_pair(stroke->getColor().name(),++i));
      stroke->setLabel(i);
    } else {
      stroke->setLabel(p_index->second);
    }
    seeds.push_back(stroke);
  }
  return seeds;
}


ZMultiscaleSegmentationWindow* ZMultiscaleSegmentationWindow::instance(){
  if(!s_window){
    s_window = new ZMultiscaleSegmentationWindow();
  }

  return s_window;
}


//implementation for ZMultiscaleSegmentationManagementModule
ZMultiscaleSegManagementModule::ZMultiscaleSegManagementModule(QObject* parent):
ZSandboxModule(parent){
  init();
}


ZMultiscaleSegManagementModule::~ZMultiscaleSegManagementModule(){
  delete ZMultiscaleSegmentationWindow::instance();
}


void ZMultiscaleSegManagementModule::init(){
  m_action = new QAction("Multiscale Segmentation", this);
  connect(m_action, SIGNAL(triggered()), this, SLOT(execute()));
}


void ZMultiscaleSegManagementModule::execute(){
  ZMultiscaleSegmentationWindow::instance()->show();
}


void ZMultiscaleSegmentationWindow::test(){
  vector<string> file_names = {"/home/deli/share/stacks/transfer/data4/seg.tif",
  "/home/deli/share/stacks/transfer/data5/seg.tif",
  "/home/deli/share/stacks/transfer/data6/seg.tif",
  "/home/deli/share/stacks/transfer/data9/seg.tif"};

  for(auto name : file_names){
    std::cout<<name<<std::endl;
    shared_ptr<ZStack> seg = shared_ptr<ZStack>(new ZStack());
    seg->load(name);
    for(uint8_t* p = seg->array8(); p < seg->array8() + seg->getVoxelNumber(); ++p){
      if(*p)*p=1;
    }
    int x0 = seg->getBoundBox().getFirstCorner().getX();
    int y0 = seg->getBoundBox().getFirstCorner().getY();
    int z0 = seg->getBoundBox().getFirstCorner().getZ();
    int x1 = seg->getBoundBox().getLastCorner().getX();
    int y1 = seg->getBoundBox().getLastCorner().getY();
    int z1 = seg->getBoundBox().getLastCorner().getZ();

    clock_t start,end;

    shared_ptr<ZSegmentationEncoder> encoder = shared_ptr<ZSegmentationEncoder>(new ZSegmentationEncoder(seg->getOffset()));

    start = std::clock();
    encoder->consume(*seg);
    end = std::clock();
    std::cout<<"Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<std::endl;

    start = std::clock();
    double sum= 0.0;
    for(int z = z0; z <= z1; ++z){
      for(int y = y0; y <= y1; ++y){
        for(int x = x0; x <= x1; ++x){
          sum += encoder->contains(x,y,z);
        }
      }
    }
    end = std::clock();
    std::cout<<"Access Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<" Check Sum: "<<sum<<std::endl;
    std::cout<<"Mem: "<<encoder->memUsage()/1024.0/1024.0<<std::endl;
  }


  //RAW
  /*shared_ptr<ZSegmentationTree> tree = std::make_shared<ZSegmentationTree>(new ZSegmentationEncoderRawFactory());
  start = std::clock();
  tree->consume(tree->getRootID(),seg);
  end = std::clock();
  std::cout<<"Raw Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<std::endl;
  start = std::clock();
  double sum= 0.0;
  for(int z = z0; z <= z1; ++z){
    for(int y = y0; y <= y1; ++y){
      for(int x = x0; x <= x1; ++x){
        sum += tree->contains(tree->getRootID(),x,y,z);
      }
    }
  }
  end = std::clock();
  std::cout<<"Access Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<" Check Sum: "<<sum<<std::endl;

  //Vec
  tree = std::make_shared<ZSegmentationTree>(new ZSegmentationEncoderRLXVectorFactory());
  start = std::clock();
  tree->consume(tree->getRootID(),seg);
  end = std::clock();
  std::cout<<"Vec Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<std::endl;
  start = std::clock();
  sum= 0.0;
  for(int z = z0; z <= z1; ++z){
    for(int y = y0; y <= y1; ++y){
      for(int x = x0; x <= x1; ++x){
        sum += tree->contains(tree->getRootID(),x,y,z);
      }
    }
  }
  end = std::clock();
  std::cout<<"Access Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<" Check Sum: "<<sum<<std::endl;

  //OSH
  tree = std::make_shared<ZSegmentationTree>(new ZSegmentationEncoderRLXOneStageHashingFactory());
  start = std::clock();
  tree->consume(tree->getRootID(),seg);
  end = std::clock();
  std::cout<<"OSH Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<std::endl;
  start = std::clock();
  sum= 0.0;
  for(int z = z0; z <= z1; ++z){
    for(int y = y0; y <= y1; ++y){
      for(int x = x0; x <= x1; ++x){
        sum += tree->contains(tree->getRootID(),x,y,z);
      }
    }
  }
  end = std::clock();
  std::cout<<"Access Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<" Check Sum: "<<sum<<std::endl;

  //TSH
  tree = std::make_shared<ZSegmentationTree>(new ZSegmentationEncoderRLXTwoStageHashingFactory());
  start = std::clock();
  tree->consume(tree->getRootID(),seg);
  end = std::clock();
  std::cout<<"TSH Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<std::endl;
  start = std::clock();
  sum= 0.0;
  for(int z = z0; z <= z1; ++z){
    for(int y = y0; y <= y1; ++y){
      for(int x = x0; x <= x1; ++x){
        sum += tree->contains(tree->getRootID(),x,y,z);
      }
    }
  }
  end = std::clock();
  std::cout<<"Access Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<" Check Sum: "<<sum<<std::endl;

  //BIT
  tree = std::make_shared<ZSegmentationTree>(new ZSegmentationEncoderBitMapFactory());
  start = std::clock();
  tree->consume(tree->getRootID(),seg);
  end = std::clock();
  std::cout<<"BMP Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<std::endl;
  start = std::clock();
  sum= 0.0;
  for(int z = z0; z <= z1; ++z){
    for(int y = y0; y <= y1; ++y){
      for(int x = x0; x <= x1; ++x){
        sum += tree->contains(tree->getRootID(),x,y,z);
      }
    }
  }
  end = std::clock();
  std::cout<<"Access Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<" Check Sum: "<<sum<<std::endl;

  //SDSCAN
  tree = std::make_shared<ZSegmentationTree>(new ZSegmentationEncoderScanFactory());
  start = std::clock();
  tree->consume(tree->getRootID(),seg);
  end = std::clock();
  std::cout<<"SCAN Construction Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<std::endl;
  start = std::clock();
  sum= 0.0;
  for(int z = z0; z <= z1; ++z){
    for(int y = y0; y <= y1; ++y){
      for(int x = x0; x <= x1; ++x){
        sum += tree->contains(tree->getRootID(),x,y,z);
      }
    }
  }
  end = std::clock();
  std::cout<<"Access Time: "<<(end-start)*1000/CLOCKS_PER_SEC<<" Check Sum: "<<sum<<std::endl;

  delete seg;*/
}
