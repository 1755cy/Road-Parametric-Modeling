#include "BridgeModeling.h"
#include <osg/LineWidth>
#include <osgDB/ReadFile>
#include <QHBoxLayout>

// 构造函数
BridgeBuilder::BridgeBuilder(QWidget* parent) : QMainWindow(parent) {
    // 初始化桥梁参数
    params = {
        5.0f,    // headLength
        100.0f,  // headElevation
        8.0f,    // deckWidth
        0.5f,    // deckThickness
        1,       // deckTextureType
        10.0f,   // pierSpacing
        3.0f,    // pierBaseWidth
        8.0f,    // pierHeight
        2        // pierTextureType
    };

    // 初始化Qt窗口
    setGeometry(100, 100, 1200, 800);
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    
    // 初始化OSG场景
    viewer = new osgViewer::Viewer();
    root = new osg::Group();
    terrainGeode = new osg::Geode();
    
    // 加载地形数据（示例使用平面）
    terrain.heightField = new osg::HeightField();
    terrain.heightField->allocate(100, 100);
    terrain.vertices = new osg::Vec3Array();
    for(int x=0; x<100; ++x) {
        for(int y=0; y<100; ++y) {
            float z = 50 + 2*sin(x/10.0)*cos(y/10.0);
            terrain.heightField->setHeight(x, y, z);
            terrain.vertices->push_back(osg::Vec3(x, y, z));
        }
    }
    
    // 执行算法流程
    computeLowLyingAreas();
    buildBridgeGeometry();
    applyTextures();
    
    // 设置场景数据
    viewer->setSceneData(root);
    viewer->realize();
}

// 步骤a: 计算低洼地带
void BridgeBuilder::computeLowLyingAreas() {
    const float A = 20.0f; // 阈值长度
    
    // 遍历地形网格
    for(int x=0; x<100; ++x) {
        for(int y=0; y<100; ++y) {
            float terrainZ = terrain.heightField->getHeight(x, y);
            float planZ = getPlanHeight(x, y);
            
            if(terrainZ < planZ) {
                lowLyingPoints.push_back(osg::Vec3(x, y, terrainZ));
            }
        }
    }
    
    // 聚类分析低洼区域（示例简化）
    if(lowLyingPoints.size() > A) {
        // 标记需要建设桥梁的区域
        qDebug() << "需要建设桥梁，低洼区域长度：" << lowLyingPoints.size();
    }
}

// 获取规划线路高度（示例使用线性插值）
float BridgeBuilder::getPlanHeight(float x, float y) {
    return 55.0f + 0.1*x + 0.05*y; // 示例线性函数
}

// 步骤b-d: 桥梁几何构建
void BridgeBuilder::buildBridgeGeometry() {
    // 计算桥头位置
    osg::Vec3 bridgeStart(20, 20, getPlanHeight(20, 20));
    osg::Vec3 bridgeHead = computeBridgeHeadPosition(bridgeStart, params.headLength);
    
    // 创建桥面几何
    createDeckGeometry();
    
    // 创建桥墩
    const float C = 15.0f; // 桥墩阈值
    if(bridgeDeckPoints.size() > C) {
        int numPiers = ceil(bridgeDeckPoints.size() / params.pierSpacing);
        for(int i=0; i<numPiers; ++i) {
            osg::Vec3 position = bridgeDeckPoints[i * params.pierSpacing];
            position.z() = params.headElevation - params.pierHeight;
            createPierGeometry(position);
        }
    }
}

// 创建桥面几何
void BridgeBuilder::createDeckGeometry() {
    osg::Geometry* deckGeom = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    
    // 生成桥面顶点（示例简化）
    for(int i=0; i<50; ++i) {
        float x = 20 + i*0.5f;
        float y = 20;
        float z = params.headElevation;
        verts->push_back(osg::Vec3(x, y, z));
        verts->push_back(osg::Vec3(x, y+params.deckWidth, z));
    }
    
    deckGeom->setVertexArray(verts);
    deckGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUAD_STRIP, 0, verts->size()));
    
    osg::Geode* deckGeode = new osg::Geode();
    deckGeode->addDrawable(deckGeom);
    root->addChild(deckGeode);
    
    // 保存桥面点用于后续计算
    for(size_t i=0; i<verts->size(); i+=2) {
        bridgeDeckPoints.push_back((*verts)[i]);
    }
}

// 创建桥墩几何
void BridgeBuilder::createPierGeometry(const osg::Vec3& position) {
    osg::Geometry* pierGeom = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    
    // 创建桥墩底座
    float half = params.pierBaseWidth / 2;
    verts->push_back(position + osg::Vec3(-half, -half, 0));
    verts->push_back(position + osg::Vec3( half, -half, 0));
    verts->push_back(position + osg::Vec3( half,  half, 0));
    verts->push_back(position + osg::Vec3(-half,  half, 0));
    
    // 创建桥墩柱体
    for(int i=0; i<4; ++i) {
        verts->push_back((*verts)[i] + osg::Vec3(0,0,params.pierHeight));
    }
    
    pierGeom->setVertexArray(verts);
    pierGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 8));
    
    osg::Geode* pierGeode = new osg::Geode();
    pierGeode->addDrawable(pierGeom);
    root->addChild(pierGeode);
    
    // 保存桥墩位置
    pierPositions.push_back(position);
}

// 步骤e: 纹理贴图
void BridgeBuilder::applyTextures() {
    // 加载纹理
    osg::Texture2D* deckTex = loadTexture(params.deckTextureType);
    osg::Texture2D* pierTex = loadTexture(params.pierTextureType);
    
    // 遍历所有几何体应用纹理
    for(auto& child : root->getChildren()) {
        osg::Geode* geode = dynamic_cast<osg::Geode*>(child.get());
        if(geode) {
            for(auto& drawable : geode->getDrawables()) {
                osg::Geometry* geom = drawable->asGeometry();
                if(geom) {
                    osg::StateSet* stateset = geom->getOrCreateStateSet();
                    if(geom->getVertexArray()->getNumElements() > 100) {
                        stateset->setTextureAttributeAndModes(0, deckTex, osg::StateAttribute::ON);
                    } else {
                        stateset->setTextureAttributeAndModes(0, pierTex, osg::StateAttribute::ON);
                    }
                }
            }
        }
    }
}

// 加载纹理资源
osg::Texture2D* BridgeBuilder::loadTexture(int type) {
    osg::Texture2D* tex = new osg::Texture2D();
    switch(type) {
        case 1: tex->setImage(osgDB::readImageFile("concrete.jpg")); break;
        case 2: tex->setImage(osgDB::readImageFile("stone.jpg")); break;
        default: tex->setImage(osgDB::readImageFile("default.png"));
    }
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    return tex;
}

// 计算桥头位置
osg::Vec3 BridgeBuilder::computeBridgeHeadPosition(const osg::Vec3& start, float length) {
    // 简化计算：沿X轴方向延长
    return osg::Vec3(start.x() + length, start.y(), start.z());
}

// Qt主函数
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    BridgeBuilder window;
    window.show();
    return app.exec();
}