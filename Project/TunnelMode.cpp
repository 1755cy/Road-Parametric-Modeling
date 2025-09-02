// TunnelModeling.cpp
#include "TunnelModeling.h"
#include <osg/LineWidth>
#include <osgDB/ReadFile>
#include <QHBoxLayout>

// 构造函数
TunnelBuilder::TunnelBuilder(QWidget* parent) : QMainWindow(parent) {
    // 初始化隧道参数
    params = {
        5.0f,    // entranceLength
        8.0f,    // entranceWidth
        4.0f,    // tunnelRadius
        0.5f,    // precision
        3.0f,    // heightThreshold
        10.0f,   // extensionLength
        1        // textureType
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
            float z = 50 + 5*sin(x/10.0)*cos(y/10.0);
            terrain.heightField->setHeight(x, y, z);
            terrain.vertices->push_back(osg::Vec3(x, y, z));
        }
    }
    
    // 执行算法流程
    computeHighGroundAreas();
    buildTunnelGeometry();
    modifyTerrain();
    
    // 设置场景数据
    viewer->setSceneData(root);
    viewer->realize();
}

// 步骤a: 计算高地地段
void TunnelBuilder::computeHighGroundAreas() {
    // 遍历地形网格
    for(int x=0; x<100; ++x) {
        for(int y=0; y<100; ++y) {
            float terrainZ = terrain.heightField->getHeight(x, y);
            float planZ = 55.0f; // 示例固定规划高度
            
            if(terrainZ > planZ + params.heightThreshold) {
                highGroundPoints.push_back(osg::Vec3(x, y, terrainZ));
            }
        }
    }
    
    // 聚类分析高地区域（示例简化）
    if(highGroundPoints.size() > 50) { // 阈值判断
        tunnelEntrance = computeTunnelEntrance();
        tunnelExit = computeTunnelExit();
    }
}

// 步骤c: 计算隧道入口位置
osg::Vec3 TunnelBuilder::computeTunnelEntrance() {
    // 找到第一个高地点作为入口
    for(const auto& pt : highGroundPoints) {
        if(pt.z() > 55.0f + params.heightThreshold) {
            return pt;
        }
    }
    return osg::Vec3(20, 20, 60); // 默认值
}

// 步骤c: 计算隧道出口位置
osg::Vec3 TunnelBuilder::computeTunnelExit() {
    // 找到最后一个高地点作为出口
    for(auto it = highGroundPoints.rbegin(); it != highGroundPoints.rend(); ++it) {
        if(it->z() > 55.0f + params.heightThreshold) {
            return *it;
        }
    }
    return osg::Vec3(80, 80, 60); // 默认值
}

// 步骤d-e: 构建隧道几何体
void TunnelBuilder::buildTunnelGeometry() {
    // 计算隧道延伸方向
    osg::Vec3 direction = tunnelExit - tunnelEntrance;
    direction.normalize();
    
    // 计算起点和终点
    osg::Vec3 start = tunnelEntrance - direction * params.extensionLength;
    osg::Vec3 end = tunnelExit + direction * params.extensionLength;
    
    // 生成隧道网格
    generateTunnelMesh(start, end);
}

// 生成隧道网格
void TunnelBuilder::generateTunnelMesh(const osg::Vec3& start, const osg::Vec3& end) {
    osg::Geometry* tunnelGeom = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    osg::Vec3Array* norms = new osg::Vec3Array();
    
    // 计算隧道长度和分段数
    float length = (end - start).length();
    int segments = length / params.precision;
    
    // 生成圆柱体隧道
    for(int i=0; i<=segments; ++i) {
        float ratio = i/(float)segments;
        osg::Vec3 pos = start + (end - start)*ratio;
        
        // 生成圆形截面
        for(float angle=0; angle<2*M_PI; angle+=M_PI/8) {
            float x = cos(angle) * params.tunnelRadius;
            float y = sin(angle) * params.tunnelRadius;
            verts->push_back(pos + osg::Vec3(x, y, 0));
            norms->push_back(osg::Vec3(x, y, 0));
        }
    }
    
    // 创建索引
    osg::DrawElementsUInt* indices = new osg::DrawElementsUInt(GL_QUADS);
    for(int i=0; i<segments; ++i) {
        for(int j=0; j<16; ++j) {
            int next = (j+1)%16;
            indices->push_back(i*16 + j);
            indices->push_back(i*16 + next);
            indices->push_back((i+1)*16 + next);
            indices->push_back((i+1)*16 + j);
        }
    }
    
    tunnelGeom->setVertexArray(verts);
    tunnelGeom->setNormalArray(norms, osg::Array::BIND_PER_VERTEX);
    tunnelGeom->addPrimitiveSet(indices);
    applyTexture(tunnelGeom);
    
    osg::Geode* tunnelGeode = new osg::Geode();
    tunnelGeode->addDrawable(tunnelGeom);
    root->addChild(tunnelGeode);
}

// 地形修改
void TunnelBuilder::modifyTerrain() {
    // 在隧道路径上挖洞
    osg::Vec3 direction = tunnelExit - tunnelEntrance;
    direction.normalize();
    float length = (tunnelExit - tunnelEntrance).length();
    
    for(float t=0; t<=length; t+=params.precision) {
        osg::Vec3 pos = tunnelEntrance + direction*t;
        carveTerrain(pos, params.tunnelRadius);
    }
}

// 地形裁剪实现
void TunnelBuilder::carveTerrain(const osg::Vec3& pos, float radius) {
    int x = pos.x();
    int y = pos.y();
    
    // 在半径范围内降低地形高度
    for(int i=-radius; i<=radius; ++i) {
        for(int j=-radius; j<=radius; ++j) {
            if(i*i + j*j <= radius*radius) {
                int xi = x + i;
                int yj = y + j;
                if(xi >=0 && xi < 100 && yj >=0 && yj < 100) {
                    terrain.heightField->setHeight(xi, yj, pos.z() - 2.0f);
                }
            }
        }
    }
    
    // 更新地形几何体
    terrain.vertices->clear();
    for(int x=0; x<100; ++x) {
        for(int y=0; y<100; ++y) {
            terrain.vertices->push_back(osg::Vec3(x, y, terrain.heightField->getHeight(x, y)));
        }
    }
    terrainGeode->removeDrawables(0, terrainGeode->getNumDrawables());
    terrainGeode->addDrawable(createTerrainGeometry());
}

// 创建地形几何体
osg::Geometry* TunnelBuilder::createTerrainGeometry() {
    osg::Geometry* geom = new osg::Geometry();
    geom->setVertexArray(terrain.vertices);
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, terrain.vertices->size()));
    return geom;
}

// 应用纹理
void TunnelBuilder::applyTexture(osg::Geometry* geom) {
    osg::Texture2D* texture = new osg::Texture2D();
    texture->setImage(osgDB::readImageFile("tunnel_texture.png"));
    
    osg::StateSet* stateset = geom->getOrCreateStateSet();
    stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
}

// Qt主函数
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TunnelBuilder window;
    window.show();
    return app.exec();
}