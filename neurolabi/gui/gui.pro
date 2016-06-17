# #####################################################################
# Automatically generated by qmake (2.01a) Mon Dec 15 16:16:33 2008
# #####################################################################
TEMPLATE = app

contains(TEMPLATE, app) {
    DEFINES += _QT_APPLICATION_
} else {
    CONFIG += staticlib
}

win32 {
    DEPLOYMENT_COMMAND = $$PWD/deploy_win.bat $(QMAKE) $$OUT_PWD

    CONFIG(release, debug|release):!isEmpty(DEPLOYMENT_COMMAND) {
        QMAKE_POST_LINK += $$DEPLOYMENT_COMMAND
    }
}

unix {
    QMAKE_PATH = $(QMAKE)
    !exists($$QMAKE_PATH) {
        QMAKE_PATH = $$[QT_INSTALL_BINS]/qmake
    }
    message("qmake path: $$QMAKE_PATH")
    exists($$QMAKE_PATH) {
        macx {
          DEPLOYMENT_COMMAND = $$PWD/deploy_mac $$QMAKE_PATH $$OUT_PWD
        }

        unix:!macx {
          DEPLOYMENT_COMMAND = $$PWD/deploy_linux $$QMAKE_PATH $$OUT_PWD
        }
    }
    CONFIG(release, debug|release):!isEmpty(DEPLOYMENT_COMMAND) {
    #    QMAKE_POST_LINK += $$DEPLOYMENT_COMMAND
    }
    message($$DEPLOYMENT_COMMAND)
    message("Post link: $$QMAKE_POST_LINK")
}

CONFIG(debug, debug|release) {
    TARGET = neuTube_d
    DEFINES += _DEBUG_ _ADVANCED_ PROJECT_PATH=\"\\\"$$PWD\\\"\"
} else {
    TARGET = neuTube
}

include(extratarget.pri)

include(extlib.pri)

# suppress warnings from 3rd party library, works for gcc and clang
QMAKE_CXXFLAGS += -isystem ../gui/ext

CONFIG += rtti exceptions

CONFIG += static_glew
CONFIG += static_gtest

DEFINES += _QT_GUI_USED_ _NEUTUBE_ HAVE_CONFIG_H _ENABLE_DDP_ _ENABLE_WAVG_

#Machine information
HOSTNAME = $$system(echo $HOSTNAME)
USER = $$system(echo $USER)
HOME = $$system(echo $HOME)
GIT = $$system(which git)

#message($$GIT)
contains(GIT, .*git) {
    COMMIT_HASH = $$system("git log --pretty=format:\"%H %p\" -1 | sed s/' '/_/g")
    DEFINES += _CURRENT_COMMIT_=\"\\\"$$COMMIT_HASH\\\"\"
    message($$COMMIT_HASH)
}

include(add_itk.pri)

#Qt5
QT += opengl xml network
isEqual(QT_MAJOR_VERSION,5) | greaterThan(QT_MAJOR_VERSION,5) {
    isEqual(QT_MAJOR_VERSION,5) {
      lessThan(QT_MINOR_VERSION,4) {
        message("Unstable Qt version $${QT_VERSION}.")
        error("Use at least Qt 5.4.0.")
      }
    }
    message("Qt 5")
    QT += concurrent gui
    DEFINES += _QT5_
    CONFIG += c++11
    QMAKE_MACOSX_DEPLOYMENT_TARGET=10.7
}

#Qt4
isEqual(QT_MAJOR_VERSION,4) {
    message("Qt 4")
}

#QT += webkit

contains(CONFIG, static_glew) { # glew from ext folder
    include($$PWD/ext/glew.pri)
} else { # use your own glew

  win32 {
    LIBS += -lglew32 -lopengl32 -lglu32
  }


  macx {
    LIBS += -lGLEW -framework AGL -framework OpenGL
  }

  unix:!macx {
    LIBS += -lGL -lGLEW -lGLU
  }
} # static glew

contains(CONFIG, static_gtest) { # gtest from ext folder
    include($$PWD/ext/gtest.pri)
}

LIBS += -lstdc++

unix {

    macx {
        DEFINES += _NEUTUBE_MAC_
        LIBS += -framework AppKit -framework IOKit \
            -framework ApplicationServices \
            -framework CoreFoundation

        ICON = images/app.icns
        QMAKE_INFO_PLIST = images/Info.plist
        QMAKE_CXXFLAGS += -m64

        exists($${NEUROLABI_DIR}/macosx10.9) {
            LIBS -= -lstdc++
            QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
            QMAKE_MAC_SDK = macosx10.9
            QMAKE_MACOSX_DEPLOYMENT_TARGET=10.9
        }

        exists($${NEUROLABI_DIR}/macosx10.10) {
            LIBS -= -lstdc++
            QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
            QMAKE_MAC_SDK = macosx10.10
            QMAKE_MACOSX_DEPLOYMENT_TARGET=10.10
        }


        doc.files = doc
        doc.path = Contents/MacOS
        QMAKE_BUNDLE_DATA += doc

#        config.files = config.xml
#        config.path = Contents/MacOS
#        QMAKE_BUNDLE_DATA += config
    } else {
        DEFINES += _NEUTUBE_LINUX_
        DEFINES += _LINUX_
        LIBS += \#-lXt -lSM -lICE \
          -lX11 -lm \
          -lpthread \
          -lGL -lrt -lGLU
        message(Checking arch...)
        contains(QMAKE_HOST.arch, x86_64) {
            message($$QMAKE_HOST.arch)
            QMAKE_CXXFLAGS += -m64
        }
        RC_FILE = images/app.icns
    }
}

win32 {
    DEFINES += _NEUTUBE_WINDOWS_
    RC_FILE = images/app.rc
}

QMAKE_CXXFLAGS += -Wno-deprecated

include(ext/QsLog/QsLog.pri)
include(ext/libqxt.pri)
include (gui_free.pri)
include(test/test.pri)

# Input
RESOURCES = gui.qrc
HEADERS += mainwindow.h \
    widgets/zimagewidget.h \
    zstackview.h \
    zimage.h \
    zstackframe.h \
    zstackpresenter.h \
    zslider.h \
    plotsettings.h \
    plotter.h \
    zinteractivecontext.h \
    ztraceproject.h \
    dialogs/channeldialog.h \
    zpunctum.h \
    dialogs/settingdialog.h \
    dialogs/frameinfodialog.h \
    dialogs/traceoutputdialog.h \
    dialogs/bcadjustdialog.h \
    dialogs/zeditswcdialog.h \
    dialogs/cannyedgedialog.h \
    dialogs/medianfilterdialog.h \
    dialogs/distancemapdialog.h \
    dialogs/regionexpanddialog.h \
    dialogs/neuroniddialog.h \
    dialogs/diffusiondialog.h \
    dialogs/connectedthresholddialog.h \
    zsinglechannelstack.h \
    dialogs/zrescaleswcdialog.h \
    zbenchtimer.h \
    zrandom.h \
    zlocsegchainconn.h \
    dialogs/flyemskeletonizationdialog.h \
    dialogs/parameterdialog.h \
    dialogs/pixelquerydialog.h \
    z3dwindow.h \
    z3dinteractionhandler.h \
    zobjsitem.h \
    zobjsmodel.h \
    z3dvolumesource.h \
    z3dvolumeraycaster.h \
    zdirectionaltemplatechain.h \
    zcolormap.h \
    zclickablelabel.h \
    zcolormapeditor.h \
    z3dcanvasrenderer.h \
    zselectfilewidget.h \
    z3dtakescreenshotwidget.h \
    z3drendererbase.h \
    z3dprimitiverenderer.h \
    z3dsphererenderer.h \
    z3dlinerenderer.h \
    z3dlinewithfixedwidthcolorrenderer.h \
    z3dconerenderer.h \
    z3dcuberenderer.h \
    zcolormapwidgetwitheditorwindow.h \
    z3dbackgroundrenderer.h \
    z3daxis.h \
    zwidgetsgroup.h \
    z3dcanvas.h \
    zspinbox.h \
    zparameter.h \
    zstringparameter.h \
    z3drenderprocessor.h \
    z3drenderport.h \
    z3dnetworkevaluator.h \
    z3dprocessor.h \
    z3dport.h \
    z3dapplication.h \
    zoptionparameter.h \
    zcombobox.h \
    znumericparameter.h \
    zspinboxwithslider.h \
    z3dcameraparameter.h \
    zeventlistenerparameter.h \
    z3dtransferfunction.h \
    z3dtransferfunctioneditor.h \
    z3dtransferfunctionwidgetwitheditorwindow.h \
    z3dvolume.h \
    z3dpickingmanager.h \
    z3dsdfont.h \
    z3dfontrenderer.h \
    z3dcanvaseventlistener.h \
    zspanslider.h \
    z3dutils.h \
    z3dmesh.h \
    zcuboid.h \
    ztest.h \
    z3dgpuinfo.h \
    z3dtexture.h \
    z3dcamera.h \
    z3dgl.h \
    zglmutils.h \
    znormcolormap.h \
    dialogs/swctypedialog.h \
    dialogs/mexicanhatdialog.h \
    z3dshaderprogram.h \
    z3dtexturecopyrenderer.h \
    z3dtextureblendrenderer.h \
    z3dtexturecoordinaterenderer.h \
    z3dvolumeraycasterrenderer.h \
    zlocsegchain.h \
    zcurve.h \
    z3dvolumeslicerenderer.h \
    zstackfile.h \
    zxmldoc.h \
    zintmap.h \
    flyem/zsegmentationanalyzer.h \
    flyem/zsegmentationbundle.h \
    flyem/zflyemstackframe.h \
    flyem/zflyemstackdoc.h \
    z3drendertarget.h \
    z3dgraph.h \
    dialogs/swcsizedialog.h \
    dialogs/swcskeletontransformdialog.h \
    zprocessprogressbase.h \
    zopencv_header.h \
    zhdf5writer.h \
    flyem/zbcfset.h \
    zmoviescene.h \
    zmovieactor.h \
    zswcmovieactor.h \
    zmoviemaker.h \
    zmoviephotographer.h \
    zmoviescript.h \
    z3drotation.h \
    zpunctamovieactor.h \
    zstackmovieactor.h \
    zmoviesceneclipper.h\
    z3darrowrenderer.h \
    zmoviecamera.h \
    z3dimage2drenderer.h \
    flyem/zflyemdatabundle.h \
    flyem/zflyemdataframe.h \
    flyemdataform.h \
    zswcobjsmodel.h \
    zpunctaobjsmodel.h \
    zobjsmanagerwidget.h \
    zmoviescriptgenerator.h \
    zmoviestage.h \
    dialogs/zalphadialog.h \
    flyem/flyemdataframeoptiondialog.h \
    z3dshadergroup.h \
    z3dcompositor.h \
    z3dgeometryfilter.h \
    z3dgraphfilter.h \
    z3dpunctafilter.h \
    z3dsurfacefilter.h \
    z3dswcfilter.h \
    z3dscene.h \
    zqtbarprogressreporter.h \
    zstackdoccommand.h \
    zcursorstore.h \
    zmessagereporter.h \
    zqtmessagereporter.h \
    zstroke2d.h \
    neutube.h \
    zreportable.h \
    dialogs/helpdialog.h \
    zswcnodeobjsmodel.h \
    zstackstatistics.h \
    zswcconnector.h \
    biocytin/biocytin.h \
    biocytin/zstackprojector.h \
    dialogs/projectiondialog.h \
    zstackviewlocator.h \
    dialogs/startsettingdialog.h \
    zstackreadthread.h \
    zswccolorscheme.h \
    dialogs/moviedialog.h \
    zpunctumio.h \
    zstatisticsutils.h \
    zswcrangeanalyzer.h \
    zellipsoid.h \
    dialogs/informationdialog.h \
    zswcnodezrangeselector.h \
    zswcnodecompositeselector.h \
    zswcnodeellipsoidrangeselector.h \
    dialogs/flyemdataquerydialog.h \
    dialogs/flyemdataprocessdialog.h \
    flyem/zstitchgrid.h \
    dialogs/autosaveswclistdialog.h \
    zswcfilelistmodel.h \
    flyem/zflyemqualityanalyzer.h \
    flyem/zintcuboidcomposition.h \
    zswcglobalfeatureanalyzer.h \
    zcommandline.h \
    zswclocationanalyzer.h \
    flyem/zflyemneuronlistmodel.h \
    flyem/zflyemneuronpresenter.h \
    biocytin/zbiocytinfilenameparser.h \
    dialogs/diagnosisdialog.h \
    zerror.h \
    zhistogram.h \
    flyem/zflyemneuronrange.h \
    flyem/zflyemneuronaxis.h \
    zswcgenerator.h \
    zpaintbundle.h \
    dialogs/flyemdataexportdialog.h \
    zflyemqueryview.h \
    dialogs/flyemgeosearchdialog.h \
    zqslogmessagereporter.h \
    dialogs/flyemgeofilterdialog.h \
    zactionactivator.h \
    zswccurvaturefeatureanalyzer.h \
    zstackdoc.h \
    zstackdocmenustore.h \
    zstackdocmenufactory.h \
    zglew.h \
    dialogs/penwidthdialog.h \
    dvid/zdvidclient.h \
    dialogs/dvidobjectdialog.h \
    zpainter.h \
    dialogs/resolutiondialog.h \
    dvid/zdvidbuffer.h \
    dvid/zdvidrequest.h \
    zmatlabprocess.h \
    zneuronseed.h \
    dialogs/dvidimagedialog.h \
    ztiledstackframe.h \
    ztilemanager.h \
    ztilemanagerview.h \
    ztilegraphicsitem.h \
    ztileinfo.h \
    flyem/zflyemneuronimagefactory.h \
    flyem/zflyemneuronfeatureanalyzer.h \
    dialogs/flyemneuronthumbnaildialog.h \
    flyem/zflyemneuronexporter.h \
    flyem/zswctreebatchmatcher.h \
    zmultitaskmanager.h \
    flyem/zflyemneuronmatchtaskmanager.h \
    flyem/zflyemneuronfiltertaskmanager.h \
    zinteractionevent.h \
    dialogs/flyemhotspotdialog.h \
    flyem/zflyemqualityanalyzertaskmanager.h \
    zworkspacefactory.h \
    dvid/zdvidreader.h \
    dialogs/flyembodyiddialog.h \
    dialogs/zdviddialog.h \
    dialogs/flyembodyfilterdialog.h \
    flyem/zskeletonizeservice.h \
    zflyemdvidreader.h \
    zstroke2darray.h \
    tilemanager.h \
    flyem/zflyemservice.h \
    zactionfactory.h \
    zstackreadfactory.h \
    zstackdoclabelstackfactory.h \
    flyem/zinteractionengine.h \
    zsparseobject.h \
    zlabelcolortable.h \
    zdocplayer.h \
    zlinesegmentobject.h \
    openvdb_header.h \
    zopenvdbobject.h \
    dialogs/flyembodysplitprojectdialog.h \
    flyem/zflyembodysplitproject.h \
    dialogs/zflyemnewbodysplitprojectdialog.h \
    zstroke2dobjsmodel.h \
    zdocplayerobjsmodel.h \
    flyem/zflyembookmarklistmodel.h \
    flyem/zflyembookmark.h \
    zabstractmodelpresenter.h \
    flyem/zflyembookmarkpresenter.h \
    flyem/zflyembookmarkarray.h \
    zstackobjectarray.h \
    zwindowfactory.h \
    dvid/zdvidwriter.h \
    dialogs/dvidskeletonizedialog.h \
    zdialogfactory.h \
    widgets/zdvidserverwidget.h \
    zwidgetfactory.h \
    zlabelededitwidget.h \
    zlabeledcombowidget.h \
    dialogs/zspinboxdialog.h \
    zbuttonbox.h \
    zkeyeventswcmapper.h \
    dialogs/zflyemroidialog.h \
    flyem/zflyemroiproject.h \
    newprojectmainwindow.h \
    zmouseeventmapper.h \
    dialogs/shapepaperdialog.h \
    zparameterarray.h \
    zframefactory.h \
    zactionbutton.h \
    dvid/zdvidbufferreader.h \
    zmouseevent.h \
    zmouseeventrecorder.h \
    zmouseeventprocessor.h \
    zstackoperator.h \
    zsleeper.h \
    dvid/libdvidheader.h \
    dialogs/dvidoperatedialog.h \
    z3dwindowfactory.h \
    zthreadfuturemap.h \
    zstackball.h \
    zstackdochittest.h \
    zkeyeventmapper.h \
    zuserinputevent.h \
    zstackobjectmanager.h \
    zstackobjectgroup.h \
    zcolorscheme.h \
    zpunctumcolorscheme.h \
    zstackpatch.h \
    zrect2d.h \
    zobjectcolorscheme.h \
    flyem/zflyembodymerger.h \
    dialogs/synapseimportdialog.h \
    flyem/zflyembodymergeproject.h \
    dialogs/flyembodymergeprojectdialog.h \
    zstackdvidgrayscalefactory.h \
    zstackdocreader.h \
    flyem/zflyembodymergedoc.h \
    dialogs/flyemprojectdialog.h \
    flyem/zflyembodymergeframe.h \
    dialogs/zsegmentationprojectdialog.h \
    zsegmentationproject.h \
    zsegmentationprojectmodel.h \
    dialogs/zsubtractswcsdialog.h \
    dialogs/zmarkswcsomadialog.h \
    zlabeledspinboxwidget.h \
    dialogs/zspinboxgroupdialog.h \
    dialogs/zautotracedialog.h \
    zstackviewmanager.h \
    zstackviewparam.h \
    zflyemprojectmanager.h \
    zflyemdataloader.h \
    dialogs/swcexportdialog.h \
    zprogressmanager.h \
    dvid/zdvidtile.h \
    dvid/zdvidresolution.h \
    dvid/zdvidtileinfo.h \
    zstackmvc.h \
    dvid/zdvidversionmodel.h \
    flyem/zflyemhackathonconfigdlg.h \
    flyem/zflyemmisc.h \
    zmessagemanager.h \
    zmessageprocessor.h \
    zmessage.h \
    zmainwindowmessageprocessor.h \
    dialogs/ztestdialog.h \
    dialogs/ztestdialog2.h \
    zstackdocloader.h \
    zstackwidget.h \
    dvid/zdvidversiondag.h \
    dvid/zdvidversion.h \
    dvid/zdvidversionnode.h \
    zbodysplitbutton.h \
    zmessagefactory.h \
    zmessagemanagermodel.h \
    zflyemcontrolform.h \
    dvid/zdvidtileensemble.h \
    dvid/zdvidlabelslice.h \
    zsttransform.h \
    zpixmap.h \
    flyem/flyemproofcontrolform.h \
    flyem/zflyemproofmvc.h \
    flyem/zflyemproofdoc.h \
    flyem/zflyembookmarkview.h \
    dvid/zdvidgrayslice.h \
    flyem/zproofreadwindow.h \
    dvid/zdvidsparsestack.h \
    flyem/flyemsplitcontrolform.h \
    flyem/zflyemmessagewidget.h \
    zwidgetmessage.h \
    zprogresssignal.h \
    zkeyeventstrokemapper.h \
    zkeyoperation.h \
    zkeyoperationmap.h \
    zkeyoperationconfig.h \
    dvid/zdvidsparsevolslice.h \
    flyem/zflyemproofpresenter.h \
    flyem/zkeyeventbodymapper.h \
    ztextmessage.h \
    ztextmessagefactory.h \
    z3dgraphfactory.h \
    zstackdochelper.h \
    flyem/zflyembodyannotationdialog.h \
    flyem/zflyemsupervisor.h \
    zkeyeventmapperfactory.h \
    zkeyoperationmapsequence.h \
    zpuncta.h \
    flyem/zdvidtileupdatetaskmanager.h \
    flyem/zpaintlabelwidget.h \
    dialogs/flyembodyinfodialog.h \
    flyem/zflyembookmarkannotationdialog.h \
    flyem/zflyembody3ddoc.h \
    z3ddef.h \
    flyem/zflyembookmarkptrarray.h \
    biocytin/zbiocytinprojectiondoc.h \
    dialogs/zflyemsplitcommitdialog.h \
    zstackdocfactory.h \
    zintcuboidobj.h \
    flyem/zflyembodywindowfactory.h \
    flyem/zflyembodycolorscheme.h \
    flyem/zflyemexternalneurondoc.h \
    ztask.h \
    flyem/zflyemqualityanalyzertask.h \
    zstackobjectpainter.h \
    flyem/zflyemkeyoperationconfig.h \
    zslicedpuncta.h \
    flyem/zflyembookmarkwidget.h \
    zmultiscalepixmap.h \
    biocytin/zbiocytinprojmaskfactory.h \
    flyem/zflyemproofdocmenufactory.h \
    flyem/zflyemsequencercolorscheme.h \
    zpunctumselector.h \
    zgraphobjsmodel.h \
    zsurfaceobjsmodel.h \
    dvid/zdvidsynapse.h \
    flyem/zflyemnamebodycolorscheme.h \
    dvid/zdvidsynapseensenmble.h \
    zcubearray.h \
    dvid/zdvidsynpasecommand.h \
    dvid/zdvidannotationcommand.h \
    dvid/zflyembookmarkcommand.h \
    misc/zstackyzview.h \
    misc/zstackyzmvc.h \
    flyem/zflyemorthowindow.h \
    flyem/zflyemorthodoc.h \
    flyem/zflyemorthomvc.h \
    flyem/zflyemorthowidget.h \
    flyem/flyemorthocontrolform.h \
    dvid/zdvidannotation.h \
    dialogs/stringlistdialog.h \
    zroiwidget.h \
    flyem/zflyemtodoitem.h \
    flyem/zflyemtodolist.h \
    flyem/zflyemtodolistfilter.h \
    flyem/zflyemtodolistmodel.h \
    flyem/zflyemtodopresenter.h \
    dialogs/flyemtododialog.h \
    zstackdocselector.h \
    flyem/zflyemproofdoccommand.h \
    flyem/zneutuservice.h \
    dialogs/flyemsettingdialog.h \
    widgets/zcolorlabel.h \
    zactionlibrary.h \
    zmenufactory.h \
    zcrosshair.h \
    zapplication.h \
    dialogs/flyemsynapsefilterdialog.h \
    flyem/zflyemmb6analyzer.h \
    dialogs/zflyemsynapseannotationdialog.h \
    dialogs/zcontrastprotocaldialog.h

FORMS += dialogs/settingdialog.ui \
    dialogs/frameinfodialog.ui \
    mainwindow.ui \
    dialogs/traceoutputdialog.ui \
    dialogs/bcadjustdialog.ui \
    dialogs/channeldialog.ui \
    dialogs/cannyedgedialog.ui \
    dialogs/medianfilterdialog.ui \
    dialogs/diffusiondialog.ui \
    dialogs/connectedthresholddialog.ui \
    dialogs/zmergeimagedialog.ui \
    dialogs/zrescaleswcdialog.ui \
    dialogs/distancemapdialog.ui \
    dialogs/regionexpanddialog.ui \
    dialogs/neuroniddialog.ui \
    dialogs/flyemskeletonizationdialog.ui \
    dialogs/parameterdialog.ui \
    dialogs/pixelquerydialog.ui \
    dialogs/swctypedialog.ui \
    dialogs/swcsizedialog.ui \
    dialogs/swcskeletontransformdialog.ui \
    dialogs/mexicanhatdialog.ui \
    dialogs/informationdialog.ui \
    flyemdataform.ui \
    dialogs/zalphadialog.ui \
    flyem/flyemdataframeoptiondialog.ui \
    dialogs/helpdialog.ui \
    dialogs/projectiondialog.ui \
    dialogs/startsettingdialog.ui \
    dialogs/moviedialog.ui \
    dialogs/flyemdataquerydialog.ui \
    dialogs/flyemdataprocessdialog.ui \
    dialogs/autosaveswclistdialog.ui \
    dialogs/diagnosisdialog.ui \
    dialogs/flyemdataexportdialog.ui \
    dialogs/flyemgeosearchdialog.ui \
    dialogs/flyemgeofilterdialog.ui \
    dialogs/penwidthdialog.ui \
    dialogs/dvidobjectdialog.ui \
    dialogs/resolutiondialog.ui \
    dialogs/dvidimagedialog.ui \
    dialogs/flyemneuronthumbnaildialog.ui \
    dialogs/flyemhotspotdialog.ui \
    dialogs/flyembodyiddialog.ui \
    dialogs/zdviddialog.ui \
    dialogs/flyembodyfilterdialog.ui \
    tilemanager.ui \
    dialogs/flyembodysplitprojectdialog.ui \
    dialogs/zflyemnewbodysplitprojectdialog.ui \
    dialogs/dvidskeletonizedialog.ui \
    dialogs/zflyemroidialog.ui \
    newprojectmainwindow.ui \
    dialogs/shapepaperdialog.ui \
    dialogs/dvidoperatedialog.ui \
    dialogs/synapseimportdialog.ui \
    dialogs/flyembodymergeprojectdialog.ui \
    dialogs/zsegmentationprojectdialog.ui \
    dialogs/zmarkswcsomadialog.ui \
    dialogs/swcexportdialog.ui \
    flyem/zflyemhackathonconfigdlg.ui \
    dialogs/ztestdialog.ui \
    dialogs/ztestdialog2.ui \
    zflyemcontrolform.ui \
    flyem/flyemproofcontrolform.ui \
    flyem/flyemsplitcontrolform.ui \
    flyem/zflyembodyannotationdialog.ui \
    dialogs/flyembodyinfodialog.ui \
    flyem/zflyembookmarkannotationdialog.ui \
    dialogs/zflyemsplitcommitdialog.ui \
    flyem/zflyembookmarkwidget.ui \
    flyem/flyemorthocontrolform.ui \
    dialogs/stringlistdialog.ui \
    dialogs/flyemtododialog.ui \
    dialogs/flyemsettingdialog.ui \
    dialogs/flyemsynapsefilterdialog.ui \
    dialogs/zflyemsynapseannotationdialog.ui \
    dialogs/zcontrastprotocaldialog.ui
SOURCES += main.cpp \
    mainwindow.cpp \
    zstackview.cpp \
    widgets/zimagewidget.cpp \
    zimage.cpp \
    zstackframe.cpp \
    zstackdoc.cpp \
    zstackpresenter.cpp \
    zslider.cpp \
    dialogs/settingdialog.cpp \
    dialogs/frameinfodialog.cpp \
    plotsettings.cpp \
    plotter.cpp \
    zinteractivecontext.cpp \
    dialogs/traceoutputdialog.cpp \
    dialogs/bcadjustdialog.cpp \
    ztraceproject.cpp \
    dialogs/channeldialog.cpp \
    zpunctum.cpp \
    dialogs/zeditswcdialog.cpp \
    dialogs/cannyedgedialog.cpp \
    zdirectionaltemplatechain.cpp \
    dialogs/medianfilterdialog.cpp \
    dialogs/diffusiondialog.cpp \
    dialogs/connectedthresholddialog.cpp \
    dialogs/zrescaleswcdialog.cpp \
    zbenchtimer.cpp \
    zrandom.cpp \
    dialogs/distancemapdialog.cpp \
    dialogs/regionexpanddialog.cpp \
    dialogs/neuroniddialog.cpp \
    dialogs/flyemskeletonizationdialog.cpp \
    dialogs/parameterdialog.cpp \
    dialogs/pixelquerydialog.cpp \
    z3dwindow.cpp \
    z3dinteractionhandler.cpp \
    zobjsitem.cpp \
    zobjsmodel.cpp \
    z3dvolumesource.cpp \
    z3dvolumeraycaster.cpp \
    zcolormap.cpp \
    zclickablelabel.cpp \
    zcolormapeditor.cpp \
    zlocsegchainconn.cpp \
    zlocsegchain.cpp \
    zcurve.cpp \
    z3dcanvasrenderer.cpp \
    zselectfilewidget.cpp \
    z3dtakescreenshotwidget.cpp \
    z3drendererbase.cpp \
    z3dprimitiverenderer.cpp \
    z3dsphererenderer.cpp \
    z3dlinerenderer.cpp \
    z3dlinewithfixedwidthcolorrenderer.cpp \
    z3dconerenderer.cpp \
    z3dcuberenderer.cpp \
    zcolormapwidgetwitheditorwindow.cpp \
    z3dbackgroundrenderer.cpp \
    z3daxis.cpp \
    zwidgetsgroup.cpp \
    z3dcanvas.cpp \
    zspinbox.cpp \
    zparameter.cpp \
    zstringparameter.cpp \
    z3drenderprocessor.cpp \
    z3drenderport.cpp \
    z3dnetworkevaluator.cpp \
    z3dprocessor.cpp \
    z3dport.cpp \
    z3dapplication.cpp \
    zcombobox.cpp \
    znumericparameter.cpp \
    zspinboxwithslider.cpp \
    z3dcameraparameter.cpp \
    zeventlistenerparameter.cpp \
    z3dtransferfunction.cpp \
    z3dtransferfunctioneditor.cpp \
    z3dtransferfunctionwidgetwitheditorwindow.cpp \
    z3dvolume.cpp \
    z3dpickingmanager.cpp \
    z3dsdfont.cpp \
    z3dfontrenderer.cpp \
    zspanslider.cpp \
    z3dutils.cpp \
    z3dmesh.cpp \
    ztest.cpp \
    z3dgpuinfo.cpp \
    z3dtexture.cpp \
    z3dcamera.cpp \
    z3dgl.cpp \
    znormcolormap.cpp \
    dialogs/swctypedialog.cpp \
    z3dshaderprogram.cpp \
    z3dtexturecopyrenderer.cpp \
    z3dtextureblendrenderer.cpp \
    z3dtexturecoordinaterenderer.cpp \
    z3dvolumeraycasterrenderer.cpp \
    z3dvolumeslicerenderer.cpp \
    flyem/zflyemstackframe.cpp \
    flyem/zflyemstackdoc.cpp \
    z3drendertarget.cpp \
    z3dgraph.cpp \
    dialogs/swcsizedialog.cpp \
    dialogs/swcskeletontransformdialog.cpp \
    zprocessprogressbase.cpp \
    dialogs/mexicanhatdialog.cpp \
    dialogs/informationdialog.cpp \
    zmoviescene.cpp \
    zmovieactor.cpp \
    zswcmovieactor.cpp \
    zmoviemaker.cpp \
    zmoviephotographer.cpp \
    zmoviescript.cpp \
    z3drotation.cpp \
    zpunctamovieactor.cpp \
    zstackmovieactor.cpp \
    zmoviesceneclipper.cpp \
    z3darrowrenderer.cpp \
    zmoviecamera.cpp \
    z3dimage2drenderer.cpp \
    flyem/zflyemdatabundle.cpp \
    flyem/zflyemdataframe.cpp \
    flyemdataform.cpp \
    zswcobjsmodel.cpp \
    zpunctaobjsmodel.cpp \
    zobjsmanagerwidget.cpp \
    zmoviescriptgenerator.cpp \
    zmoviestage.cpp \
    dialogs/zalphadialog.cpp \
    flyem/flyemdataframeoptiondialog.cpp \
    z3dshadergroup.cpp \
    z3dcompositor.cpp \
    z3dgeometryfilter.cpp \
    z3dgraphfilter.cpp \
    z3dpunctafilter.cpp \
    z3dsurfacefilter.cpp \
    z3dswcfilter.cpp \
    z3dscene.cpp \
    zqtbarprogressreporter.cpp \
    zstackdoccommand.cpp \
    zcursorstore.cpp \
    zqtmessagereporter.cpp \
    zstroke2d.cpp \
    dialogs/helpdialog.cpp \
    zswcnodeobjsmodel.cpp \
    biocytin/biocytin.cpp \
    dialogs/projectiondialog.cpp \
    zstackviewlocator.cpp \
    dialogs/startsettingdialog.cpp \
    zstackreadthread.cpp \
    zswccolorscheme.cpp \
    dialogs/moviedialog.cpp \
    zpunctumio.cpp \
    dialogs/flyemdataquerydialog.cpp \
    dialogs/flyemdataprocessdialog.cpp \
    dialogs/autosaveswclistdialog.cpp \
    zswcfilelistmodel.cpp \
    zcommandline.cpp \
    flyem/zflyemneuronlistmodel.cpp \
    flyem/zflyemneuronpresenter.cpp \
    neutube.cpp \
    dialogs/diagnosisdialog.cpp \
    zpaintbundle.cpp \
    dialogs/flyemdataexportdialog.cpp \
    zflyemqueryview.cpp \
    dialogs/flyemgeosearchdialog.cpp \
    zqslogmessagereporter.cpp \
    dialogs/flyemgeofilterdialog.cpp \
    zactionactivator.cpp \
    zswccurvaturefeatureanalyzer.cpp \
    zstackdocmenustore.cpp \
    zstackdocmenufactory.cpp \
    dialogs/penwidthdialog.cpp \
    dvid/zdvidclient.cpp \
    dialogs/dvidobjectdialog.cpp \
    zpainter.cpp \
    dialogs/resolutiondialog.cpp \
    dvid/zdvidbuffer.cpp \
    dvid/zdvidrequest.cpp \
    zmatlabprocess.cpp \
    zneuronseed.cpp \
    dialogs/dvidimagedialog.cpp \
    ztiledstackframe.cpp \
    ztilemanager.cpp \
    ztilemanagerview.cpp \
    ztilegraphicsitem.cpp \
    ztileinfo.cpp \
    flyem/zflyemneuronimagefactory.cpp \
    flyem/zflyemneuronfeatureanalyzer.cpp \
    dialogs/flyemneuronthumbnaildialog.cpp \
    flyem/zflyemneuronexporter.cpp \
    flyem/zswctreebatchmatcher.cpp \
    zmultitaskmanager.cpp \
    flyem/zflyemneuronmatchtaskmanager.cpp \
    flyem/zflyemneuronfiltertaskmanager.cpp \
    zinteractionevent.cpp \
    dialogs/flyemhotspotdialog.cpp \
    flyem/zflyemqualityanalyzertaskmanager.cpp \
    zworkspacefactory.cpp \
    dvid/zdvidreader.cpp \
    dialogs/flyembodyiddialog.cpp \
    dialogs/zdviddialog.cpp \
    dialogs/flyembodyfilterdialog.cpp \
    flyem/zskeletonizeservice.cpp \
    zflyemdvidreader.cpp \
    zstroke2darray.cpp \
    tilemanager.cpp \
    flyem/zflyemservice.cpp \
    zactionfactory.cpp \
    zstackreadfactory.cpp \
    zstackdoclabelstackfactory.cpp \
    flyem/zinteractionengine.cpp \
    zsparseobject.cpp \
    zlabelcolortable.cpp \
    zdocplayer.cpp \
    zlinesegmentobject.cpp \
    zopenvdbobject.cpp \
    dialogs/flyembodysplitprojectdialog.cpp \
    flyem/zflyembodysplitproject.cpp \
    dialogs/zflyemnewbodysplitprojectdialog.cpp \
    zstroke2dobjsmodel.cpp \
    zdocplayerobjsmodel.cpp \
    flyem/zflyembookmarklistmodel.cpp \
    flyem/zflyembookmark.cpp \
    zabstractmodelpresenter.cpp \
    flyem/zflyembookmarkpresenter.cpp \
    flyem/zflyembookmarkarray.cpp \
    zstackobjectarray.cpp \
    zwindowfactory.cpp \
    dvid/zdvidwriter.cpp \
    dialogs/dvidskeletonizedialog.cpp \
    zdialogfactory.cpp \
    widgets/zdvidserverwidget.cpp \
    zwidgetfactory.cpp \
    zlabelededitwidget.cpp \
    zlabeledcombowidget.cpp \
    dialogs/zspinboxdialog.cpp \
    zbuttonbox.cpp \
    zkeyeventswcmapper.cpp \
    dialogs/zflyemroidialog.cpp \
    flyem/zflyemroiproject.cpp \
    newprojectmainwindow.cpp \
    zmouseeventmapper.cpp \
    dialogs/shapepaperdialog.cpp \
    zparameterarray.cpp \
    zframefactory.cpp \
    zactionbutton.cpp \
    dvid/zdvidbufferreader.cpp \
    zmouseevent.cpp \
    zmouseeventrecorder.cpp \
    zmouseeventprocessor.cpp \
    zstackoperator.cpp \
    zsleeper.cpp \
    dialogs/dvidoperatedialog.cpp \
    z3dwindowfactory.cpp \
    zthreadfuturemap.cpp \
    zstackball.cpp \
    zstackdochittest.cpp \
    zkeyeventmapper.cpp \
    zuserinputevent.cpp \
    zstackobjectmanager.cpp \
    zstackobjectgroup.cpp \
    zcolorscheme.cpp \
    zpunctumcolorscheme.cpp \
    zstackpatch.cpp \
    zrect2d.cpp \
    zobjectcolorscheme.cpp \
    flyem/zflyembodymerger.cpp \
    dialogs/synapseimportdialog.cpp \
    flyem/zflyembodymergeproject.cpp \
    dialogs/flyembodymergeprojectdialog.cpp \
    zstackdvidgrayscalefactory.cpp \
    zstackdocreader.cpp \
    flyem/zflyembodymergedoc.cpp \
    dialogs/flyemprojectdialog.cpp \
    flyem/zflyembodymergeframe.cpp \
    dialogs/zsegmentationprojectdialog.cpp \
    zsegmentationproject.cpp \
    zsegmentationprojectmodel.cpp \
    dialogs/zsubtractswcsdialog.cpp \
    dialogs/zmarkswcsomadialog.cpp \
    zlabeledspinboxwidget.cpp \
    dialogs/zspinboxgroupdialog.cpp \
    dialogs/zautotracedialog.cpp \
    zstackviewmanager.cpp \
    zstackviewparam.cpp \
    zflyemprojectmanager.cpp \
    zflyemdataloader.cpp \
    dialogs/swcexportdialog.cpp \
    zprogressmanager.cpp \
    dvid/zdvidtile.cpp \
    dvid/zdvidresolution.cpp \
    dvid/zdvidtileinfo.cpp \
    zstackmvc.cpp \
    dvid/zdvidversionmodel.cpp \
    flyem/zflyemhackathonconfigdlg.cpp \
    flyem/zflyemmisc.cpp \
    zmessagemanager.cpp \
    zmessageprocessor.cpp \
    zmessage.cpp \
    zmainwindowmessageprocessor.cpp \
    dialogs/ztestdialog.cpp \
    dialogs/ztestdialog2.cpp \
    zstackdocloader.cpp \
    zstackwidget.cpp \
    dvid/zdvidversiondag.cpp \
    dvid/zdvidversion.cpp \
    dvid/zdvidversionnode.cpp \
    zbodysplitbutton.cpp \
    zmessagefactory.cpp \
    zmessagemanagermodel.cpp \
    zflyemcontrolform.cpp \
    dvid/zdvidtileensemble.cpp \
    dvid/zdvidlabelslice.cpp \
    zsttransform.cpp \
    zpixmap.cpp \
    flyem/flyemproofcontrolform.cpp \
    flyem/zflyemproofmvc.cpp \
    flyem/zflyemproofdoc.cpp \
    flyem/zflyembookmarkview.cpp \
    dvid/zdvidgrayslice.cpp \
    flyem/zproofreadwindow.cpp \
    dvid/zdvidsparsestack.cpp \
    flyem/flyemsplitcontrolform.cpp \
    flyem/zflyemmessagewidget.cpp \
    zwidgetmessage.cpp \
    zprogresssignal.cpp \
    zkeyeventstrokemapper.cpp \
    zkeyoperation.cpp \
    zkeyoperationmap.cpp \
    zkeyoperationconfig.cpp \
    dvid/zdvidsparsevolslice.cpp \
    flyem/zflyemproofpresenter.cpp \
    flyem/zkeyeventbodymapper.cpp \
    ztextmessage.cpp \
    ztextmessagefactory.cpp \
    z3dgraphfactory.cpp \
    zstackdochelper.cpp \
    flyem/zflyembodyannotationdialog.cpp \
    flyem/zflyemsupervisor.cpp \
    zkeyeventmapperfactory.cpp \
    zkeyoperationmapsequence.cpp \
    zpuncta.cpp \
    flyem/zdvidtileupdatetaskmanager.cpp \
    flyem/zpaintlabelwidget.cpp \
    dialogs/flyembodyinfodialog.cpp \
    flyem/zflyembookmarkannotationdialog.cpp \
    flyem/zflyembody3ddoc.cpp \
    z3ddef.cpp \
    flyem/zflyembookmarkptrarray.cpp \
    biocytin/zbiocytinprojectiondoc.cpp \
    dialogs/zflyemsplitcommitdialog.cpp \
    zstackdocfactory.cpp \
    zintcuboidobj.cpp \
    flyem/zflyembodywindowfactory.cpp \
    flyem/zflyembodycolorscheme.cpp \
    flyem/zflyemexternalneurondoc.cpp \
    ztask.cpp \
    flyem/zflyemqualityanalyzertask.cpp \
    zstackobjectpainter.cpp \
    flyem/zflyemkeyoperationconfig.cpp \
    zslicedpuncta.cpp \
    flyem/zflyembookmarkwidget.cpp \
    zmultiscalepixmap.cpp \
    biocytin/zbiocytinprojmaskfactory.cpp \
    flyem/zflyemproofdocmenufactory.cpp \
    flyem/zflyemsequencercolorscheme.cpp \
    zpunctumselector.cpp \
    zgraphobjsmodel.cpp \
    zsurfaceobjsmodel.cpp \
    dvid/zdvidsynapse.cpp \
    flyem/zflyemnamebodycolorscheme.cpp \
    dvid/zdvidsynapseensenmble.cpp \
    zcubearray.cpp \
    dvid/zdvidsynpasecommand.cpp \
    dvid/zdvidannotationcommand.cpp \
    dvid/zflyembookmarkcommand.cpp \
    misc/zstackyzview.cpp \
    misc/zstackyzmvc.cpp \
    flyem/zflyemorthowindow.cpp \
    flyem/zflyemorthodoc.cpp \
    flyem/zflyemorthomvc.cpp \
    flyem/zflyemorthowidget.cpp \
    flyem/flyemorthocontrolform.cpp \
    dvid/zdvidannotation.cpp \
    dialogs/stringlistdialog.cpp \
    zroiwidget.cpp \
    flyem/zflyemtodoitem.cpp \
    flyem/zflyemtodolist.cpp \
    flyem/zflyemtodolistfilter.cpp \
    flyem/zflyemtodolistmodel.cpp \
    flyem/zflyemtodopresenter.cpp \
    dialogs/flyemtododialog.cpp \
    zstackdocselector.cpp \
    flyem/zflyemproofdoccommand.cpp \
    flyem/zneutuservice.cpp \
    dialogs/flyemsettingdialog.cpp \
    widgets/zcolorlabel.cpp \
    zactionlibrary.cpp \
    zmenufactory.cpp \
    zcrosshair.cpp \
    zapplication.cpp \
    dialogs/flyemsynapsefilterdialog.cpp \
    flyem/zflyemmb6analyzer.cpp \
    dialogs/zflyemsynapseannotationdialog.cpp \
    dialogs/zcontrastprotocaldialog.cpp

OTHER_FILES += \
    extlib.pri \
    extratarget.pri
