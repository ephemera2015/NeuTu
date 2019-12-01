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
#include <QFileInfo>
#include <QLineEdit>
#include <set>
#include <map>
#include "time.h"
#include "zmultiscalesegmentationmanagement.h"
#include "zstackobject.h"
#include "flyem/zstackwatershedcontainer.h"
#include "zobject3dscanarray.h"
#include "mvc/zstackframe.h"
#include "zsandbox.h"
#include "zstackdocdatabuffer.h"
#include "imgproc/zstackprocessor.h"
#include "imgproc/zwatershedmst.h"
#include "mvc/zstackdoc.h"
#include "segment/zsegmentationnodewrapper.h"
#include "mainwindow.h"
#include "widgets/zpythonprocess.h"
#include "zstroke2d.h"
#include "zobject3d.h"


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

  //ZColorScheme scheme;
  //scheme.setColorScheme(ZColorScheme::UNIQUE_COLOR);
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
    //QColor color = scheme.getColor(i++);
    //color.setAlpha(50);
    //wrapper->setColor(color);
    wrapper->setObjectId(t);
    wrapper->addCallBackOnSelection(onSelectMask);
    wrapper->addCallBackOnDeselection(onDeselectMask);
    wrapper->addRole(ZStackObjectRole::ROLE_SEGMENTATION);
    wrapper->setSelectable(true);
    if(m_show_contour->isChecked()){
      wrapper->setDisplayStyle(ZStackObject::EDisplayStyle::BOUNDARY);
    } else {
      wrapper->setDisplayStyle(ZStackObject::EDisplayStyle::SOLID);
    }
    m_frame->document()->getDataBuffer()->addUpdate(wrapper, ZStackDocObjectUpdate::EAction::ADD_NONUNIQUE);
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
  QPushButton* create_super_voxels = new QPushButton("Create Super Voxels");
  QPushButton* segment = new QPushButton("Segment");
  QPushButton* grow = new QPushButton("Grow");
  QPushButton* grow_d = new QPushButton("Grow_d");
  QPushButton* ffn = new QPushButton("FFN");
  QPushButton* merge = new QPushButton("Merge");
  QPushButton* export_leaves = new QPushButton("Export Leaves");
  QPushButton* export_selected = new QPushButton("Export Selected");
  //QPushButton* create_super_voxels = new QPushButton("Create Super Voxels");
  m_show_leaves = new QCheckBox("Show Leaves");
  //m_enable_super_voxel = new QCheckBox("Super Voxel");
  m_show_contour =new QCheckBox("Show Contour");

  //m_merge_from = new QLineEdit();
  //m_merge_to = new QLineEdit();

  //setup layout
  QGridLayout* lay=new QGridLayout(this);
  lay->addWidget(m_view,0,0,8,20);

  //lay->addWidget(m_enable_super_voxel,9,0,1,4);
  lay->addWidget(m_show_contour,9,0,1,4);
  lay->addWidget(m_show_leaves,9,4,1,4);

  lay->addWidget(open_stack,10,0,1,4);
  lay->addWidget(create_super_voxels,10,4,1,4);
  lay->addWidget(segment,10,8,1,4);
  lay->addWidget(grow,10,12,1,4);
  lay->addWidget(merge,10,16,1,4);

  lay->addWidget(export_leaves,11,0,1,4);
  lay->addWidget(export_selected,11,4,1,4);
  lay->addWidget(grow_d,11,8,1,4);
  lay->addWidget(ffn,11,12,1,4);
  //lay->addWidget(create_super_voxels,11,0,1,5);

  /*lay->addWidget(new QLabel("From:"),11,0,1,2);
  lay->addWidget(m_merge_from,11,2,1,3);
  lay->addWidget(new QLabel("To:"),11,5,1,1);
  lay->addWidget(m_merge_to,11,6,1,3);*/


  connect(open_stack,SIGNAL(clicked()),this,SLOT(onOpenStack()));
  connect(segment,SIGNAL(clicked()),this,SLOT(onSegment()));
  connect(grow,SIGNAL(clicked()),this,SLOT(onGrow()));
  connect(grow_d,SIGNAL(clicked()),this,SLOT(onGrowDownsampled()));
  connect(ffn,SIGNAL(clicked()),this,SLOT(onFFN()));
  connect(merge,SIGNAL(clicked()),this,SLOT(onMerge()));
  connect(export_leaves,SIGNAL(clicked()),this,SLOT(onExportLeaves()));
  connect(export_selected,SIGNAL(clicked()),this,SLOT(onExportSelected()));
  connect(m_view,SIGNAL(itemClicked(QTreeWidgetItem*,int)),this,SLOT(onNodeItemClicked(QTreeWidgetItem*,int)));
  connect(m_show_leaves,SIGNAL(stateChanged(int)),this,SLOT(onShowLeaves(int)));
  connect(create_super_voxels,SIGNAL(clicked()),this,SLOT(onCreateSuperVoxels()));

  this->setLayout(lay);
  this->setMinimumSize(400,600);
  this->move(300,200);
}


ZMultiscaleSegmentationWindow::~ZMultiscaleSegmentationWindow(){

}


shared_ptr<ZStack> ZMultiscaleSegmentationWindow::_createSuperVoxel(const ZStack &stack, uint& start_label,int sx, int sy, int sz) const {
  shared_ptr<ZStack> rv = shared_ptr<ZStack>(new ZStack(FLOAT32, stack.getBoundBox(), 1));
  std::pair<bool,shared_ptr<ZStack>> status_seeds = _seedsFromLocalMaximum(stack,sx,sy,sz);

  if(status_seeds.first){
     ZStackWatershedContainer container(const_cast<ZStack*>(&stack));
     container.setDsMethod("Min(ignore zero)");
     container.addSeed(*(status_seeds.second));
     container.run();

     ZStackPtr presult = container.getResultStack();
     uint8_t* src = presult->array8();
     float* dst = rv->array32();
     std::set<int> labels;
     for(uint i = 0; i < presult->getVoxelNumber(); ++i){
      if(src[i]){
        dst[i] = src[i] + start_label - 1;
        labels.insert(src[i]);
      }
     }
     start_label += labels.size();
  } else {
    ZIntCuboid box = stack.getBoundBox();
    ZIntCuboid box_a = box;
    ZIntCuboid box_b = box;
    int width = box.getWidth();
    int height = box.getHeight();
    int depth = box.getDepth();
    if(height >= width && height >= depth){
      box_a.setLastY(box.getFirstCorner().getY() + height/2);
      box_b.setFirstY(box.getFirstCorner().getY() + height/2 + 1);
    } else if(width >= height && width >= depth){
      box_a.setLastX(box.getFirstCorner().getX() + width/2);
      box_b.setFirstX(box.getFirstCorner().getX() + width/2 + 1);
    } else {
      box_a.setLastZ(box.getFirstCorner().getZ() + depth/2);
      box_b.setFirstZ(box.getFirstCorner().getZ() + depth /2 + 1);
    }
    shared_ptr<ZStack> stack_a = shared_ptr<ZStack>(stack.makeCrop(box_a));
    shared_ptr<ZStack> stack_b = shared_ptr<ZStack>(stack.makeCrop(box_b));

    shared_ptr<ZStack> rv_a = _createSuperVoxel(*stack_a,start_label,sx,sy,sz);
    shared_ptr<ZStack> rv_b = _createSuperVoxel(*stack_b,start_label,sx,sy,sz);

    int x0 = rv_a->getOffset().getX();
    int y0 = rv_a->getOffset().getY();
    int z0 = rv_a->getOffset().getZ();
    rv->setBlockValue(x0,y0,z0,rv_a.get());
    int x1 = rv_b->getOffset().getX();
    int y1 = rv_b->getOffset().getY();
    int z1 = rv_b->getOffset().getZ();
    rv->setBlockValue(x1,y1,z1,rv_b.get());
  }
  return rv;
}

std::pair<bool, shared_ptr<ZStack>> ZMultiscaleSegmentationWindow::_seedsFromLocalMaximum(const ZStack &stack,int sx, int sy, int sz) const{
  shared_ptr<ZStack> rv = shared_ptr<ZStack>(stack.clone());
  rv->setZero();

  int width = stack.width();
  int height = stack.height();
  int depth  = stack.depth();
  int area = width * height;

  const uint8_t* p = stack.array8();
  uint8_t* q = rv->array8();
  uint k = 1;

  int stride_x = sx;
  int stride_y = sy;
  int stride_z = sz;

  for(int d = 1; d < depth - 1; d += stride_z){
    for(int h = 1; h < height - 1; h += stride_y){
      size_t index = 1 + h * width + d * area;
      for(int w = 1; w < width - 1; w += stride_x, index += stride_x){
        bool ok = true;
        uint8 v = p[index];
        if(v){
          for(int k = -1; k <= 1; ++k){
            for(int j = -1; j <= 1; ++j){
              for(int i = -1; i <= 1; ++i){
                if( i==0 && j==0 && k==0){
                  continue;
                }
                if(p[index + k*area + j*width +i] >= v){
                  ok = false;
                  break;
                }
              }
            }
          }
          if(ok){
            if( k > 255){
              return std::make_pair(false,nullptr);
            }
            q[index] = k++;
            //w += 1;
            //index += 1;
          }
        }
      }
    }
  }
  return std::make_pair(true,rv);
}


void ZMultiscaleSegmentationWindow::onCreateSuperVoxels(){
  if(!m_frame || ! m_stack){
    return;
  }

  shared_ptr<ZStack> stack = shared_ptr<ZStack>(makeSelectedStack());
  uint start_label = 1;
  int stride_x = std::max(1,stack->width()/40);
  int stride_y = std::max(1,stack->height()/40);
  int stride_z = std::max(1,stack->depth()/40);
  shared_ptr<ZStack> label_stack = _createSuperVoxel(*stack,start_label,stride_x,stride_y,stride_z);

  m_seg_tree->consume(m_selected_id,*label_stack);
}


void ZMultiscaleSegmentationWindow::onExportLeaves(){
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


void ZMultiscaleSegmentationWindow::onExportSelected(){
  if(!m_stack){
    return;
  }
  shared_ptr<ZStack> segmentation = shared_ptr<ZStack>(m_stack->clone());
  //segmentation->setZero();
  m_seg_tree->maskStack(m_selected_id,segmentation.get());

  QString file_name = QFileDialog::getSaveFileName(this,"Save Segmentation","","tiff(*.tif)");
  if(file_name != ""){
    if(!file_name.endsWith(".tif")){
      file_name = file_name + ".tif";
    }
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
  QList<ZSegmentationNodeWrapper*> list = m_frame->document()->getSelectedObjectList<ZSegmentationNodeWrapper>(
                                          ZStackObject::EType::SEGMENTATION_ENCODER);
  vector<string> from_ids;
  string to = m_selected_id;
  for(auto it = list.begin(); it != list.end(); ++it){
    string from = (*it)->getNodeID();
    from_ids.push_back(from);
  }
  m_seg_tree->merge(from_ids,to);
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

  if(m_seg_tree->isLeaf(m_selected_id) ||
    (m_selected_id == m_seg_tree->getRootID() &&
    m_seg_tree->getChildrenIDs(m_selected_id).size() == 0)) {
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
    if(stack->getVoxelNumber() > 1e8){
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
  } else {
    onSuperVoxel();
  }
}


void ZMultiscaleSegmentationWindow::onGrow(){
  if(!m_frame || !m_stack){
    return;
  }

  const QString working_dir = QCoreApplication::applicationDirPath() + "/../python/service/inter_segnet";

  m_stack->save(QString(working_dir + "/data.tif").toStdString());

  std::shared_ptr<ZStack> mask = std::shared_ptr<ZStack>(m_stack->clone());
  if(m_selected_id == m_seg_tree->getRootID()){
    mask->setOne();
  } else {
    mask->setZero();
    m_seg_tree->labelStack(m_selected_id,mask.get(),1);
  }
  mask->save(QString(working_dir + "/mask.tif").toStdString());

  std::vector<ZObject3d*> rseeds;
  std::vector<ZObject3d*> bseeds;

  QList<ZStackObject*> objList = m_frame->document()->getObjectList(ZStackObjectRole::ROLE_SEED);

  for (ZStackObject* stroke : objList) {
    ZStroke2d* p = dynamic_cast<ZStroke2d*>(stroke);
    if(stroke->getColor().name() == "#ff0000"){
      stroke->setLabel(1);
      rseeds.push_back(p->toObject3d());
    } else if(stroke->getColor().name() == "#00ff00"){
      stroke->setLabel(1);
      bseeds.push_back(p->toObject3d());
    }
  }

  shared_ptr<ZStack> rseed_stack = shared_ptr<ZStack>(toSeedStack(rseeds,m_stack->width(),m_stack->height(),m_stack->depth(),m_stack->getOffset()));
  shared_ptr<ZStack> bseed_stack = shared_ptr<ZStack>(toSeedStack(bseeds,m_stack->width(),m_stack->height(),m_stack->depth(),m_stack->getOffset()));

  rseed_stack->save(QString(working_dir + "/rseeds.tif").toStdString());
  bseed_stack->save(QString(working_dir + "/bseeds.tif").toStdString());

  ZPythonProcess python;
  python.setWorkDir(working_dir);
  python.setScript(working_dir + "/main.py");
  python.run();

  shared_ptr<ZStack> result = shared_ptr<ZStack>(new ZStack());

  if(QFileInfo(QString(working_dir + "/result.tif")).isFile()){
    result->load(QString(working_dir + "/result.tif").toStdString());
    if(result->hasData()){
      for(uint i = 0; i < result->getVoxelNumber(); ++i){
        if(mask->array8()[i]){
          result->array8()[i] += 1;
        }
      }
      result->setOffset(m_stack->getOffset());
      m_seg_tree->consume(m_selected_id,result.get());//here
    }
  }
  removeSeeds();
}


void ZMultiscaleSegmentationWindow::onFFN(){
  if(!m_frame || !m_stack){
    return;
  }
  std::shared_ptr<ZStack> mask = std::shared_ptr<ZStack>(m_stack->clone());

  if(m_selected_id == m_seg_tree->getRootID()){
    mask->setOne();
  } else {
    mask->setZero();
    m_seg_tree->labelStack(m_selected_id,mask.get(),1);
  }

  std::vector<ZObject3d*> rseeds;
  QList<ZStackObject*> objList = m_frame->document()->getObjectList(ZStackObjectRole::ROLE_SEED);

  for (ZStackObject* stroke : objList) {
    ZStroke2d* p = dynamic_cast<ZStroke2d*>(stroke);
    stroke->setLabel(1);
    rseeds.push_back(p->toObject3d());
  }
  shared_ptr<ZStack> rseed_stack = shared_ptr<ZStack>(toSeedStack(rseeds,m_stack->width(),m_stack->height(),m_stack->depth(),m_stack->getOffset()));
  const QString working_dir=QCoreApplication::applicationDirPath()+"/../python/service/ffn";
  m_stack->save(working_dir.toStdString()+"/data.tif");
  std::ofstream out(working_dir.toStdString()+"/seed.txt");
  for(int k=0;k<rseed_stack->depth();++k){
    for(int j=0;j<rseed_stack->height();++j){
      for(int i=0;i<rseed_stack->width();++i){
        if(rseed_stack->array8()[i+j*rseed_stack->width()+k*rseed_stack->width()*rseed_stack->height()]){
            out<<k<<" "<<j<<" "<<i<<std::endl;
         }
       }
     }
   }
  ZPythonProcess python;
  python.setWorkDir(working_dir);
  python.setScript(working_dir+"/ffn_skeleton.py");
  python.run();

  std::shared_ptr<ZStack> result = std::shared_ptr<ZStack>(new ZStack());
  result->load(working_dir.toStdString()+"/result.tif");
  if(result && result->hasData()){
    for(uint i = 0; i < mask->getVoxelNumber(); ++i){
      if(mask->array8()[i]){
        result->array8()[i] += 1;
      } else {
        result->array8()[i] = 0;
      }
    }
    result->setOffset(m_stack->getOffset());
    m_seg_tree->consume(m_selected_id,result.get());//here
  }
  removeSeeds();
}


void ZMultiscaleSegmentationWindow::onGrowDownsampled(){
  if(!m_frame || !m_stack){
    return;
  }

  const QString working_dir = QCoreApplication::applicationDirPath() + "/../python/service/inter_segnet";


  shared_ptr<ZStack> dstack = shared_ptr<ZStack>(m_stack->clone());
  dstack->downsampleMinIgnoreZero(1,1,1);
  //dstack->downsampleMean(1,1,1);
  dstack->save(QString(working_dir + "/data.tif").toStdString());

  std::shared_ptr<ZStack> mask = std::shared_ptr<ZStack>(m_stack->clone());
  if (m_selected_id == m_seg_tree->getRootID()){
    mask->setOne();
  } else {
    mask->setZero();
    m_seg_tree->labelStack(m_selected_id,mask.get(),1);
  }

  std::shared_ptr<ZStack> mask_d = std::shared_ptr<ZStack>(mask->clone());
  mask_d->downsampleMax(1,1,1);
  mask_d->save(QString(working_dir + "/mask.tif").toStdString());

  std::vector<ZObject3d*> rseeds;
  std::vector<ZObject3d*> bseeds;

  QList<ZStackObject*> objList = m_frame->document()->getObjectList(ZStackObjectRole::ROLE_SEED);

  for (ZStackObject* stroke : objList) {
    ZStroke2d* p = dynamic_cast<ZStroke2d*>(stroke);
    if(stroke->getColor().name() == "#ff0000"){
      stroke->setLabel(1);
      rseeds.push_back(p->toObject3d());
    } else if(stroke->getColor().name() == "#00ff00"){
      stroke->setLabel(1);
      bseeds.push_back(p->toObject3d());
    }
  }

  shared_ptr<ZStack> rseed_stack = shared_ptr<ZStack>(toSeedStack(rseeds,m_stack->width(),m_stack->height(),m_stack->depth(),m_stack->getOffset()));
  shared_ptr<ZStack> bseed_stack = shared_ptr<ZStack>(toSeedStack(bseeds,m_stack->width(),m_stack->height(),m_stack->depth(),m_stack->getOffset()));

  rseed_stack->downsampleMax(1,1,1);
  bseed_stack->downsampleMax(1,1,1);

  rseed_stack->save(QString(working_dir + "/rseeds.tif").toStdString());
  bseed_stack->save(QString(working_dir + "/bseeds.tif").toStdString());

  ZPythonProcess python;
  python.setWorkDir(working_dir);
  python.setScript(working_dir + "/main_d.py");
  python.run();

  shared_ptr<ZStack> result = shared_ptr<ZStack>(new ZStack());

  if(QFileInfo(QString(working_dir + "/result.tif")).isFile()){
    result->load(QString(working_dir + "/result.tif").toStdString());
    if(result->hasData()){
      shared_ptr<ZStack> tmp = shared_ptr<ZStack>(m_stack->clone());
      Upsample_Stack(result->c_stack(),1,1,1,tmp->c_stack());
      for(uint i = 0; i < mask->getVoxelNumber(); ++i){
        if(mask->array8()[i]){
          tmp->array8()[i] += 1;
        }
      }
      //result->setOffset(stack->getOffset());
      tmp->setOffset(m_stack->getOffset());
      //m_seg_tree->consume(m_selected_id,result.get());//here
      m_seg_tree->consume(m_selected_id,tmp.get());//here
    }
  }
  removeSeeds();
}


ZStack* ZMultiscaleSegmentationWindow::toSeedStack(std::vector<ZObject3d*>& seeds,int width,int height,int depth,ZIntPoint offset){
  ZStack* mask=new ZStack(GREY,width,height,depth,1);
  mask->setOffset(offset);
  for(ZObject3d* seed:seeds){
    uint8_t label = seed->getLabel();
    uint8_t *array = mask->array8();
    size_t area = width*height;
    for (size_t i = 0; i < seed->size(); ++i){
      int x = seed->getX(i);
      int y = seed->getY(i);
      int z = seed->getZ(i);
      //x /= m_scale;
      //y /= m_scale;
      //z /= m_scale;
      x -= offset.getX();
      y -= offset.getY();
      z -= offset.getZ();
      if (IS_IN_CLOSE_RANGE(x, 0, width - 1) &&
          IS_IN_CLOSE_RANGE(y, 0, height - 1) &&
          IS_IN_CLOSE_RANGE(z, 0, depth - 1)) {
        array[z * area + y * width + x] = label;
      }
    }
  }
  return mask;
}


void ZMultiscaleSegmentationWindow::onSuperVoxel(){
  shared_ptr<ZStack> stack = shared_ptr<ZStack>(makeSelectedStack());
  if(!stack){
    return;
  }


  shared_ptr<ZStack> label = shared_ptr<ZStack>(new ZStack(FLOAT32,stack->getBoundBox(),1));
  vector<string> ids = m_seg_tree->getChildrenIDs(m_selected_id);

  if(ids.size() < 2){
    return ;
  }

  int index = 1;
  map<int,int> index_to_label;

  for(const string& id: ids){
    m_seg_tree->labelStack(id,*label,index);
    index_to_label[index] = m_seg_tree->getLabel(id);
    index++;
  }
  ZMSTContainer container;

  vector<int> result;
  container.run(result,*stack, getSeeds(),label);

  map<int,vector<int>> groups;
  for(uint i = 0; i < result.size(); ++i){
    groups[result[i]].push_back(index_to_label[i+1]);
  }

  if(groups.size()){
    m_seg_tree->group(m_selected_id,groups);
  }
  //if(rv){
  //  m_seg_tree->consume(m_selected_id,*rv);
  //}

  removeSeeds();
}


void ZMultiscaleSegmentationWindow::onOpenStack(){
  QString file_name = QFileDialog::getOpenFileName(this,"Open Stack","","tiff(*.tif)");
  if (file_name!=""){
    if(m_frame && m_frame == ZSandbox::GetCurrentFrame()){
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
  m_frame->document()->removeObject(ZStackObjectRole::ROLE_SEED);
  //for(ZStroke2d* stroke:m_frame->document()->getStrokeList()){
  //  m_frame->document()->removeObject(stroke);
  //}
}


std::vector<ZStackObject*> ZMultiscaleSegmentationWindow::getSeeds(){
  if(!m_frame){
    return std::vector<ZStackObject*>();
  }

  std::vector<ZStackObject*> seeds;
  std::map<QString,int> color_indices;
  int i = 0;

  QList<ZStackObject*> objList = m_frame->document()->getObjectList(ZStackObjectRole::ROLE_SEED);

  for (ZStackObject* stroke : objList) {
    std::map<QString,int>::iterator p_index =
        color_indices.find(stroke->getColor().name());

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
