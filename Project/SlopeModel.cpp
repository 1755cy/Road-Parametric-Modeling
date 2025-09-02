#include "SlopeModeling.h"
#include <osg/LineWidth>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <QHBoxLayout>
#include <cmath>

// 构造函数
SlopeModeler::SlopeModeler(QWidget* parent) : QMainWindow(parent) {
    // 初始化边坡参数
    params = {
        100.0f,   // baseElevation
        30.0f,    // slopeAngle
        2,        // stages
        5.0f,     // platformWidth
        2.0f,     // ditchWidth
        1.0f      // gridSize
    };

    // 初始化Qt窗口
    setGeometry(100, 100, 1200, 800);
    QWidget* centralWidget = new QWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    
    // 初始化OSG场景
    viewer = new osgViewer::Viewer();
    root = new osg::Group();
    terrainGeode = new osg::Geode();
    
    // 添加地形（示例使用平面）
    osg::Geometry* terrain = new osg::Geometry();
    osg::Vec3Array* terrainVerts = new osg::Vec3Array();
    for(float x=-50; x<=50; x+=1) {
        for(float y=-50; y<=50; y+=1) {
            terrainVerts->push_back(osg::Vec3(x, y, 100 + 5*sin(x/5)*cos(y/5)));
        }
    }
    terrain->setVertexArray(terrainVerts);
    terrain->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, terrainVerts->size()));
    terrainGeode->addDrawable(terrain);
    root->addChild(terrainGeode);
    
    // 执行算法流程
    computeSlopeRange();
    gridSegmentation();
    unitClassification();
    build3DBlocks();
    validateAndMerge();
    
    // 设置场景数据
    viewer->setSceneData(root);
    viewer->realize();
}

// 步骤1：计算边坡范围
void SlopeModeler::computeSlopeRange() {
    // 1.1 创建基本横断面
    osg::ref_ptr<osg::Geometry> crossSection = createCrossSection(0);
    
    // 1.2 沿纵向推进计算交点
    for(float offset = 0; offset < 100; offset += 5.0f) {
        computeIntersections(createCrossSection(offset));
    }
    
    // 1.3 创建拓扑面
    createTopologySurface();
}

// 创建横断面几何体
osg::Geometry* SlopeModeler::createCrossSection(float offset) {
    osg::Geometry* geom = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    
    // 计算横断面点（示例简化）
    float angleRad = osg::DegreesToRadians(params.slopeAngle);
    float currentHeight = params.baseElevation;
    
    for(int i=0; i<=params.stages; ++i) {
        float platformZ = currentHeight + params.platformWidth * tan(angleRad);
        verts->push_back(osg::Vec3(offset, -50, currentHeight));
        verts->push_back(osg::Vec3(offset, 50, platformZ));
        currentHeight = platformZ;
    }
    
    geom->setVertexArray(verts);
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, verts->size()));
    return geom;
}

// 计算地形交点
void SlopeModeler::computeIntersections(osg::Geometry* crossSection) {
    // 获取地形顶点
    osg::Vec3Array* terrainVerts = dynamic_cast<osg::Vec3Array*>(terrainGeode->getDrawable(0)->asGeometry()->getVertexArray());
    
    // 遍历横断面线段
    osg::Vec3Array* csVerts = dynamic_cast<osg::Vec3Array*>(crossSection->getVertexArray());
    for(size_t i=0; i<csVerts->size()-1; ++i) {
        osg::Vec3 p1 = (*csVerts)[i];
        osg::Vec3 p2 = (*csVerts)[i+1];
        
        // 遍历地形三角形（示例简化）
        for(size_t j=0; j<terrainVerts->size()-2; j+=3) {
            osg::Vec3 t1 = (*terrainVerts)[j];
            osg::Vec3 t2 = (*terrainVerts)[j+1];
            osg::Vec3 t3 = (*terrainVerts)[j+2];
            
            // 计算线段与三角形交点（简化实现）
            osg::Vec3 intersect;
            if(/* 交点检测逻辑 */) {
                intersections.push_back({intersect, false});
            }
        }
    }
}

// 创建拓扑面
void SlopeModeler::createTopologySurface() {
    osg::Geometry* surface = new osg::Geometry();
    osg::Vec3Array* verts = new osg::Vec3Array();
    
    // 按顺时针排序交点
    // 此处需要实现凸包算法或排序逻辑
    for(const auto& pt : intersections) {
        verts->push_back(pt.point);
    }
    
    surface->setVertexArray(verts);
    surface->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON, 0, verts->size()));
    
    osg::Geode* geode = new osg::Geode();
    geode->addDrawable(surface);
    root->addChild(geode);
}

// 步骤2：格网分割
void SlopeModeler::gridSegmentation() {
    // 获取范围面包围盒
    osg::BoundingBox bb;
    for(const auto& pt : intersections) {
        bb.expandBy(pt.point);
    }
    
    // 创建格网
    int xSteps = ceil((bb.xMax() - bb.xMin()) / params.gridSize);
    int ySteps = ceil((bb.yMax() - bb.yMin()) / params.gridSize);
    
    gridUnits.resize(ySteps, std::vector<MicroUnit>(xSteps));
    
    // 填充格网单元
    for(int y=0; y<ySteps; ++y) {
        for(int x=0; x<xSteps; ++x) {
            float x0 = bb.xMin() + x*params.gridSize;
            float y0 = bb.yMin() + y*params.gridSize;
            float x1 = x0 + params.gridSize;
            float y1 = y0 + params.gridSize;
            
            // 创建四边形单元（示例简化）
            gridUnits[y][x] = createMicroUnit(
                osg::Vec3(x0, y0, 100),
                osg::Vec3(x1, y0, 100),
                osg::Vec3(x1, y1, 100),
                osg::Vec3(x0, y1, 100)
            );
        }
    }
}

// 创建微分单元
MicroUnit SlopeModeler::createMicroUnit(const osg::Vec3& v1, const osg::Vec3& v2, 
                                      const osg::Vec3& v3, const osg::Vec3& v4) {
    MicroUnit unit;
    unit.vertices[0] = v1;
    unit.vertices[1] = v2;
    unit.vertices[2] = v3;
    unit.vertices[3] = v4;
    unit.property = 0; // 默认属性
    return unit;
}

// 步骤3：单元分类
void SlopeModeler::unitClassification() {
    // 遍历所有单元进行分类
    for(auto& row : gridUnits) {
        for(auto& unit : row) {
            // 计算单元中心点
            osg::Vec3 center = (unit.vertices[0] + unit.vertices[2]) * 0.5f;
            
            // 根据位置判断属性（示例简化）
            if(center.y() < params.ditchWidth) {
                unit.property = 1; // 排水沟
            } else if(center.z() > params.baseElevation + 10) {
                unit.property = 2; // 一级边坡
            }
            // 添加更多分类规则...
        }
    }
}

// 步骤4：构建三维体块
void SlopeModeler::build3DBlocks() {
    // 合并相同属性单元
    mergeAdjacentUnits();
    
    // 创建三维体块
    for(const auto& block : slopeBlocks) {
        osg::Geode* geode = new osg::Geode();
        geode->addDrawable(block.geometry.get());
        applyTexture(block.geometry.get(), block.property);
        root->addChild(geode);
    }
}

// 合并相邻单元
void SlopeModeler::mergeAdjacentUnits() {
    // 实现基于空间索引的合并算法
    // 此处需要实现四叉树或网格索引加速查找
    for(int y=0; y<gridUnits.size()-1; ++y) {
        for(int x=0; x<gridUnits[y].size()-1; ++x) {
            // 查找相邻相同属性单元
            if(gridUnits[y][x].property == gridUnits[y][x+1].property) {
                // 合并为更大单元
                MicroUnit merged = createMicroUnit(
                    gridUnits[y][x].vertices[0],
                    gridUnits[y][x+1].vertices[1],
                    gridUnits[y][x+1].vertices[2],
                    gridUnits[y][x].vertices[3]
                );
                merged.property = gridUnits[y][x].property;
                // 添加到体块集合...
            }
        }
    }
}

// 应用纹理
void SlopeModeler::applyTexture(osg::Geometry* geom, int property) {
    osg::Texture2D* texture = new osg::Texture2D();
    switch(property) {
        case 1: texture->setImage(osgDB::readImageFile("ditch.png")); break;
        case 2: texture->setImage(osgDB::readImageFile("slope1.png")); break;
        // 添加更多纹理...
    }
    
    osg::StateSet* stateset = geom->getOrCreateStateSet();
    stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
}

// 步骤5：验证合并
void SlopeModeler::validateAndMerge() {
    // 实现相邻面距离计算
    // 遍历所有体块进行合并
    for(auto it1 = slopeBlocks.begin(); it1 != slopeBlocks.end(); ++it1) {
        for(auto it2 = std::next(it1); it2 != slopeBlocks.end(); ) {
            // 计算面间距离
            float distance = /* 计算逻辑 */;
            if(distance < 0.1f) {
                // 合并体块
                it1->geometry->addPrimitiveSet(it2->geometry->getPrimitiveSet(0));
                it2 = slopeBlocks.erase(it2);
            } else {
                ++it2;
            }
        }
    }
}

// Qt主函数
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    SlopeModeler window;
    window.show();
    return app.exec();
}