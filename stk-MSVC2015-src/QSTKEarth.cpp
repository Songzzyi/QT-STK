#include "QSTKEarth.h"
#include <QDateTime>    // 获取当前时间
#include <QDebug>

//#include <comutil.h> // For _bstr_t

// 月份中文转英文映射
QString monthToEnglish(int month)
{
    static const QStringList months =
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return months[month - 1];
}

// 静态成员变量初始化
QMutex QSTKEarth::mutex;
QAtomicPointer<QSTKEarth> QSTKEarth::instance = 0;
int QSTKEarth::SateIndex = 0;  // 初始化全局卫星数
int QSTKEarth::UEIndex = 0;    // 初始化全局用户数
int QSTKEarth::SenIndex = 0;   // 初始化全局传感器数

QSTKEarth::QSTKEarth(QWidget* parent)
    : QWidget(parent)
{
    // HRESULT Windows API中的一种数据类型，用于表示函数调用的返回状态。
    HRESULT hr = ::CoInitialize(NULL); // 初始化 COM 库
    if (FAILED(hr))
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("系统提示"), QString::fromLocal8Bit("初始化 COM 库失败！"));
        return;
    }

    hr = m_app.CreateInstance(__uuidof(AgSTKXApplication)); // 创建一个STKX应用程序对象
    if (FAILED(hr))
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("系统提示"), QString::fromLocal8Bit("创建程序对象失败！"));
        return;
    }

    hr = m_pRoot.CreateInstance(__uuidof(AgStkObjectRoot)); // 创建一个STK根对象
    if (FAILED(hr))
    {
        QMessageBox::warning(this, QString::fromLocal8Bit("系统提示"), QString::fromLocal8Bit("创建根对象失败！"));
        return;
    }
    enableControl = false;
    isPlayForward = true;   // 构造中默认前向播放
    notFirstInit = false;
}

QSTKEarth::~QSTKEarth()
{
    m_pRoot.Release();
    m_app.Release();
    ::CoUninitialize(); // 反初始化COM库，释放COM库的资源
}

void QSTKEarth::NewScenario()
{
    if(notFirstInit)
    {
        UnloadStkScence();
    }
    notFirstInit = true;
    // Q_ASSERT() 是一个宏，用于在调试模式下进行断言检查。
    Q_ASSERT(m_app != NULL);
    STKXLib::IAgSTKXApplicationPtr pSTKXapp(m_app);
    pSTKXapp->ExecuteCommand("Unload / *"); // 卸载所有当前加载的场景和对象
    pSTKXapp->ExecuteCommand("New / Scenario ScenOne"); // 创建一个新的场景，命名为 ScenOne
    enableControl = true;
    // 仿真时间设置为3年后
    ScenarioStartTime = timeManager.getCurrentTime();
    ScenarioStopTime = timeManager.getTimeAfterYears(3, 0, 0);  //（3年，0月，0日）
//    qDebug() << ScenarioStartTime << endl;
//    qDebug() << ScenarioStopTime << endl;

    IAgScenarioPtr scenario = m_pRoot->CurrentScenario;
    scenario->SetTimePeriod(_bstr_t(ScenarioStartTime.toStdString().c_str()), _bstr_t(ScenarioStopTime.toStdString().c_str()));
    Rewind();
}

//void QSTKEarth::LoadScenario() // 加载场景
//{
//    Q_ASSERT(m_pRoot != NULL);
//    m_pRoot->CloseScenario();
//    m_pRoot->LoadScenario(_bstr_t("..\\data\\Scenario1.sc"));
//    enableControl = true;
//}

void QSTKEarth::PauseSTK()
{
    if (enableControl)
    {
        Q_ASSERT(m_app != NULL);
        STKXLib::IAgSTKXApplicationPtr pSTKXapp(m_app);
        pSTKXapp->ExecuteCommand("Animate * Pause");
        //    pSTKXapp->Pause();//也可以直接调用类成员函数
    }
}

void QSTKEarth::FasterSTK()
{
    if (enableControl)
    {
        Q_ASSERT(m_app != NULL);
        STKXLib::IAgSTKXApplicationPtr pSTKXapp(m_app);
        pSTKXapp->ExecuteCommand("Animate * Faster");
    }
}

void QSTKEarth::SlowerSTK()
{
    if (enableControl)
    {
        Q_ASSERT(m_app != NULL);
        STKXLib::IAgSTKXApplicationPtr pSTKXapp(m_app);
        pSTKXapp->ExecuteCommand("Animate * Slower");
    }
}

void QSTKEarth::ResetSTK()
{
    if (enableControl)
    {
        Q_ASSERT(m_pRoot != NULL);
        STKObjects::IAgAnimationPtr pAnimation(m_pRoot);
        pAnimation->Rewind();
    }
}

void QSTKEarth::UnloadStkScence() // 卸载场景
{
    Q_ASSERT(m_app != NULL);
    STKXLib::IAgSTKXApplicationPtr pSTKXapp(m_app);
    pSTKXapp->ExecuteCommand("UnloadMulti / */Satellite/*");
    pSTKXapp->ExecuteCommand("UnloadMulti / */Missile/*");
    pSTKXapp->ExecuteCommand("Unload / *");
    enableControl = false;
    // 重置全局计数器
    this->SateIndex = 0;
    this->UEIndex = 0;
    this->SenIndex = 0;
}

// 随机添加卫星
void QSTKEarth::AddRandomSatellite()
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to create STK Object Root. Create Satellite Failed!");
    }

    char buf[255];
    double aporad, perrad;  // 远/近地点
    _bstr_t bstrName;
    IAgOrbitStateClassicalPtr pClassical;   // 指向经典轨道状态对象
    IAgStkObjectPtr pNewObj;    // 指向新创建的STK对象
    IAgSatellitePtr pSat;   // 指向卫星对象
    IAgVePropagatorTwoBodyPtr pTwoBodyProp; // 指向二体传播器对象
    IAgClassicalSizeShapeRadiusPtr pRadius; // 指向轨道的半径对象

    // 生成唯一卫星名称
    _itoa_s(this->SateIndex++, buf, 10);  // 使用_itoa_s替代itoa,因为itoa 函数已被弃用(警告)
    bstrName = "Satellite";
    bstrName += buf;
    // 创建新的卫星对象 并添加到场景中
    pNewObj = m_pRoot->GetCurrentScenario()->GetChildren()->New(AgESTKObjectType::eSatellite, bstrName);
    pSat = pNewObj;
    pSat->SetPropagatorType(AgEVePropagatorType::ePropagatorTwoBody);
    pTwoBodyProp = pSat->Propagator;
    pClassical = pTwoBodyProp->InitialState->Representation->ConvertTo(AgEOrbitStateType::eOrbitStateClassical);    // 将初始状态的表示转换为经典轨道状态
    pClassical->SizeShapeType = STKUtil::eSizeShapeRadius;  // 轨道状态的大小形状为半径类型
    pRadius = pClassical->SizeShape;
    // 生成随机的轨道参数
    std::uniform_real_distribution<double> aporadDist(0.0, 12000.0);
    std::uniform_real_distribution<double> perradDist(0.0, 12000.0);
    std::uniform_real_distribution<double> inclinationDist(0.0, 180.0);
    aporad = aporadDist(rng);   // 远地点
    perrad = perradDist(rng);    // 近地点
    pRadius->ApogeeRadius = 6356.75231424 + aporad; // 近地点半径
    pRadius->PerigeeRadius = 6356.75231424 + perrad;    // 远地点半径
    pClassical->Orientation->Inclination = inclinationDist(rng);    // 轨道倾角

    pTwoBodyProp->InitialState->Representation->Assign(pClassical);
    pTwoBodyProp->Propagate();

    /************************** 添加传感器 **************************/
    char                      senBuf[255];
    _bstr_t                   bstrSenName;
    _itoa_s(this->SenIndex++, senBuf, 10);
    bstrSenName = "Sensor";
    bstrSenName += senBuf;
    STKObjects::IAgStkObjectPtr pSensorObj = pSat;
    STKObjects::IAgSensorPtr pSen = pSensorObj->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName);

    IAgSnSimpleConicPatternPtr patternData = pSen->CommonTasks->SetPatternSimpleConic("1.1", 0.1);
    pSen->Graphics->Color = 0x000000ff; // 传感器照射颜色设置为红色
    pSen->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/0");
    // 设置传感器“凝视”，盯着名为0这个地面站。前提是必须已经有这么个叫0的地面站
    //"*/Facility/Facility0"--->"*/Facility/Facility0" 是 STK 中的路径表示法，表示位于当前场景中的一个名为 Facility0 的地面站
}

// 添加带有传感器的卫星
void QSTKEarth::AddSatelliteWithSensor()
{
    if(m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to create STK Object Root. Create Satellite Failed!");
    }
    char                      satBuf[255];
    char                      senBuf[255];
    _bstr_t                   bstrSatName;
    _bstr_t                   bstrSenName;
    _itoa_s(this->SateIndex++, satBuf, 10);
    _itoa_s(this->SenIndex++, senBuf, 10);
    bstrSatName = "Satellite";
    bstrSenName = "Sensor";
    bstrSatName += satBuf;
    bstrSenName += senBuf;

    // 创建卫星并设置其轨道参数
    STKObjects::IAgStkObjectPtr pNewObj = m_pRoot->GetCurrentScenario()->GetChildren()->New(AgESTKObjectType::eSatellite, bstrSatName);
    STKObjects::IAgSatellitePtr pSat = pNewObj;

    pSat->SetPropagatorType(AgEVePropagatorType::ePropagatorTwoBody);
    STKObjects::IAgVePropagatorTwoBodyPtr twoBody = pSat->Propagator;
    //_variant_t startTime = "25 Mar 2021 12:00:00.000";
    //_variant_t stopTime = "25 Mar 2021 12:00:00.000";
    // twoBody->StopTime 等属性期望的是 _variant_t 类型
    // 注意：这里起止时间应该得合适，不然报错？还未进一步探究
    //twoBody->StartTime = startTime;
    //twoBody->StopTime = stopTime;
    //twoBody->put_StopTime(stopTime);
    //twoBody->Step = 3;

    twoBody->InitialState->Representation->AssignClassical(STKUtil::AgECoordinateSystem(eCoordinateSystemJ2000), 590.0 + 6378.0, 0.0, 30.0, 0, 90.0, 0.0);
    // 第一个参数：使用的坐标系，这里用的是eCoordinateSystemJ2000也就是 J2000 惯性坐标系
    // 第二个参数：轨道的半长轴（单位：km）= 轨道高度+地球半径(约6378km)
    // 第三个参数：轨道离心率（0表示圆形轨道，介于0和1之间表示椭圆轨道）
    // 第四个参数：轨道的倾角（单位：度degrees），轨道平面相对于参考平面的倾斜角度
    // 第五个参数：升交点赤经（RAAN,单位：度degrees），轨道平面与参考平面的交点（升交点）在参考平面上的投影与参考方向（通常是春分点）之间的角度
    // 第六个参数：近地点辐角（单位：度degrees），轨道上最近地球点（近地点）与升交点之间的角度
    // 第七个参数：真近点角（单位：度degrees），轨道上卫星位置与近地点之间的角度
    try
    {
        twoBody->Propagate();
    }
    catch (_com_error& e)
    {
        qDebug() << "Propagate failed: " << e.ErrorMessage();
        return;
    }

    /************************** 添加传感器 **************************/
    // 创建一个传感器并设置其属性
    //STKObjects::IAgStkObjectPtr pSensorObj = pNewObj->GetChildren()->New(AgESTKObjectType::eSensor,bstrSatName);
    STKObjects::IAgStkObjectPtr pSensorObj = pSat;
    STKObjects::IAgSensorPtr pSen = pSensorObj->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName);

    IAgSnSimpleConicPatternPtr patternData = pSen->CommonTasks->SetPatternSimpleConic("1.1", 0.1);
    pSen->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/Facility0");
    // 前提是必须已经有这么个叫Facility0的地面站，
    //"*/Facility/Facility0"--->"*/Facility/Facility0" 是 STK 中的路径表示法，表示位于当前场景中的一个名为 Facility0 的地面站
}

// 随机添加地面站
void QSTKEarth::AddRamdonFacility()
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to create STK Object Root.");
    }

    char                 buf[255];
    _bstr_t              bstrName;
    IAgStkObjectPtr      pNewObj;
    IAgFacilityPtr       pFacility;
    IAgGeodeticPtr       pPos;

    _itoa_s(this->UEIndex++, buf, 10);
    bstrName = "Facility";
    bstrName += buf;
    // 创建新的设施对象，并添加到场景中
    pNewObj = m_pRoot->GetCurrentScenario()->GetChildren()->New(AgESTKObjectType::eFacility, bstrName);
    pFacility =  pNewObj;
    pPos = pFacility->Position->ConvertTo( eGeodetic );

    // srand(static_cast<unsigned int>(time(NULL)));   // 初始化随机种子
    // 关于随机种子：随机数生成器在生成随机数时依赖于一个初始值，称为种子（seed）
    // time(NULL)：返回当前时间的秒数，因此短时间内返回的秒数很接近，相当于用同一个随机数种子，故生成的随机数也会很接近，可以使用高精度的种子
    std::uniform_real_distribution<double> latDist(-90.0, 90.0);
    std::uniform_real_distribution<double> lonDist(0.0, 360.0);
    double lat, lon;
    bool positionExists = true;    // 标志位判断这个坐标是否已经有地面设施
    // 不重合生成地面设施
    while(positionExists)
    {
        lat = latDist(rng);    // 纬度：-90到90度
        lon = lonDist(rng);    // 经度：0到360度
        positionExists = false;
        for(std::vector<std::pair<double, double>>::iterator it = existingFacilities.begin(); it != existingFacilities.end(); it++)
        {
            if(it->first == lat && it->second == lon)
            {
                positionExists = true;
                break;
            }
        }
    }
    existingFacilities.push_back(std::make_pair(lat, lon));
    pPos->Lon = lon;
    pPos->Lat = lat;
    pFacility->Position->Assign( pPos );

//    pPos->Lat = rand() % 181 - 90;  // 纬度：-90° ~ 90°
//    pPos->Lon = rand() % 361;       // 经度：0° ~ 360°
//    pFacility->Position->Assign( pPos );
}

// 根据经纬度添加地面站
void QSTKEarth::AddFacility(double lat, double lon)
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to create STK Object Root.");
    }

    char                 buf[255];
    _bstr_t              bstrName;
    IAgStkObjectPtr      pNewObj;
    IAgFacilityPtr       pFacility;
    IAgGeodeticPtr       pPos;

    _itoa_s(this->UEIndex++, buf, 10);
//    bstrName = "Facility";
    bstrName = "";
    bstrName += buf;
    // 创建新的设施对象，并添加到场景中
    pNewObj = m_pRoot->GetCurrentScenario()->GetChildren()->New(AgESTKObjectType::eFacility, bstrName);
    pFacility =  pNewObj;
    pPos = pFacility->Position->ConvertTo( eGeodetic );

    pPos->Lon = lon;
    pPos->Lat = lat;
    pFacility->Position->Assign( pPos );
}

/******************** 动画演示部分 ********************/
void QSTKEarth::PlayForward()
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to PlayForward.");
    }
    // assert(m_pRoot != NULL); // 断言机制
    STKObjects::IAgAnimationPtr pAnimation( m_pRoot );
    pAnimation->PlayForward();
}

void QSTKEarth::PlayBackward()
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to PlayBackward.");
    }
    STKObjects::IAgAnimationPtr pAnimation( m_pRoot );
    pAnimation->PlayBackward();
}

void QSTKEarth::Pause()
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to Pause.");
    }
    STKObjects::IAgAnimationPtr pAnimation( m_pRoot );
    pAnimation->Pause();
}

void QSTKEarth::Rewind()
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to Rewind.");
    }
    STKObjects::IAgAnimationPtr pAnimation( m_pRoot );
    pAnimation->Rewind();
}

void QSTKEarth::ZoomIn()
{
    // 咋做
}

/******************** 场景创建部分 ********************/

// 波束开关显示 - onOff：true 显示波束，false 不显示波束
void QSTKEarth::SenserioOnOff(const QString& satelliteName, const QString& sensorName, bool onOff)
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to execute command. Root object is null.");
    }
    QString cmd;
    if(onOff)
    {
        cmd = QString("Graphics */Satellite/%1/Sensor/%2 Show On").arg(satelliteName).arg(sensorName);
    }
    else
    {
        cmd = QString("Graphics */Satellite/%1/Sensor/%2 Show Off").arg(satelliteName).arg(sensorName);
    }

    try
    {
        Q_ASSERT(m_app != NULL);
        STKXLib::IAgSTKXApplicationPtr pSTKXapp(m_app);
        IAgExecCmdResultPtr result = pSTKXapp->ExecuteCommand(_bstr_t(cmd.toStdString().c_str()));
//        qDebug() << "Executing command: " << cmd;
    }
    catch (_com_error& e)
    {
        qDebug() << "ExecuteCommand failed: " << e.ErrorMessage();
    }

    qDebug() << "Sensor beam display turned off successfully.";
}

// 卫星与地面站收发包表示 - 连线
void QSTKEarth::SatelliteUEConnect(const QString& satelliteName, const QString& facilityName, bool onOff)
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to execute command. Root object is null.");
    }
    QString cmd;
    if(onOff)
    {
        cmd = QString("Access */Satellite/%1/ */Facility/%2").arg(satelliteName).arg(facilityName);
    }
    else
    {
        cmd = QString("RemoveAccess */Satellite/%1/ */Facility/%2").arg(satelliteName).arg(facilityName);
    }

    try
    {
        Q_ASSERT(m_app != NULL);
        STKXLib::IAgSTKXApplicationPtr pSTKXapp(m_app);
        IAgExecCmdResultPtr result = pSTKXapp->ExecuteCommand(_bstr_t(cmd.toStdString().c_str()));
        qDebug() << "Executing command: " << cmd;
    }
    catch (_com_error& e)
    {
        qDebug() << "ExecuteCommand failed: " << e.ErrorMessage();
    }
}

// 10 用户 1 卫星
// 配置特定轨道参数
// 添加一个宽波束，对应10个用户添加对应凝视传感器
// 初始化关闭宽波束与凝视传感器的显示
void QSTKEarth::AddSpecificSatellite()
{
    if (m_pRoot == NULL)
    {
        throw std::runtime_error("Failed to create STK Object Root. Create Satellite Failed!");
    }
    // 卫星命名
    //char satBuf[255];
    _bstr_t bstrSatName;
    //_itoa_s(this->SateIndex++, satBuf, 10);
    bstrSatName = "Sat1";
    //bstrSatName += satBuf;
    // 宽波束传感器
    //char senBuf[255];
    _bstr_t bstrSenName;
    //_itoa_s(this->SenIndex++, senBuf, 10);
    bstrSenName = "SensorWideBeam"; // 宽波束就叫"SensorWideBeam",无序号
    //bstrSenName += senBuf;
    // 凝视传感器0
    char senBuf0[255];
    _bstr_t bstrSenName0;
    _itoa_s(this->SenIndex++, senBuf0, 10);
    bstrSenName0 = "Sen";
    bstrSenName0 += senBuf0;
    // 凝视传感器1
    char senBuf1[255];
    _bstr_t bstrSenName1;
    _itoa_s(this->SenIndex++, senBuf1, 10);
    bstrSenName1 = "Sen";
    bstrSenName1 += senBuf1;
    // 凝视传感器2
    char senBuf2[255];
    _bstr_t bstrSenName2;
    _itoa_s(this->SenIndex++, senBuf2, 10);
    bstrSenName2 = "Sen";
    bstrSenName2 += senBuf2;
    // 凝视传感器3
    char senBuf3[255];
    _bstr_t bstrSenName3;
    _itoa_s(this->SenIndex++, senBuf3, 10);
    bstrSenName3 = "Sen";
    bstrSenName3 += senBuf3;
    // 凝视传感器4
    char senBuf4[255];
    _bstr_t bstrSenName4;
    _itoa_s(this->SenIndex++, senBuf4, 10);
    bstrSenName4 = "Sen";
    bstrSenName4 += senBuf4;
    // 凝视传感器5
    char senBuf5[255];
    _bstr_t bstrSenName5;
    _itoa_s(this->SenIndex++, senBuf5, 10);
    bstrSenName5 = "Sen";
    bstrSenName5 += senBuf5;
    // 凝视传感器6
    char senBuf6[255];
    _bstr_t bstrSenName6;
    _itoa_s(this->SenIndex++, senBuf6, 10);
    bstrSenName6 = "Sen";
    bstrSenName6 += senBuf6;
    // 凝视传感器7
    char senBuf7[255];
    _bstr_t bstrSenName7;
    _itoa_s(this->SenIndex++, senBuf7, 10);
    bstrSenName7 = "Sen";
    bstrSenName7 += senBuf7;
    // 凝视传感器8
    char senBuf8[255];
    _bstr_t bstrSenName8;
    _itoa_s(this->SenIndex++, senBuf8, 10);
    bstrSenName8 = "Sen";
    bstrSenName8 += senBuf8;
    // 凝视传感器9
    char senBuf9[255];
    _bstr_t bstrSenName9;
    _itoa_s(this->SenIndex++, senBuf9, 10);
    bstrSenName9 = "Sen";
    bstrSenName9 += senBuf9;
    /*************************************************/
    // 定义卫星与参数
    STKObjects::IAgStkObjectPtr pNewObj = m_pRoot->GetCurrentScenario()->GetChildren()->New(AgESTKObjectType::eSatellite, bstrSatName);
    STKObjects::IAgSatellitePtr pSat = pNewObj;
    pSat->SetPropagatorType(AgEVePropagatorType::ePropagatorTwoBody);
    STKObjects::IAgVePropagatorTwoBodyPtr twoBody = pSat->Propagator;

    _variant_t startTime(ScenarioStartTime.toStdString().c_str());
    _variant_t stopTime(ScenarioStopTime.toStdString().c_str());
    twoBody->PutStartTime(startTime);
    twoBody->PutStopTime(stopTime);
    twoBody->Step = 60; // 设置时间步长，单位为秒
    // 设置同步轨道参数，正下方为中国（东经103度）
    double semiMajorAxis = 42164.0; // 地心距42164公里
    double inclination = 0.116286; // 轨道倾角0度
    double raan = 0; // 升交点赤经（对同步轨道无实际影响）
    double argumentOfPerigee = 60; // 近地点幅角0度
    double trueAnomaly = -30; // 真近点角0度

    twoBody->InitialState->Representation->AssignClassical(
        STKUtil::AgECoordinateSystem(eCoordinateSystemJ2000),
        semiMajorAxis,       // 轨道半长轴
        1.3971e-016,                 // 离心率
        inclination,         // 轨道倾角
        raan,                // 升交点赤经
        argumentOfPerigee,   // 近地点幅角
        trueAnomaly   // 真近点角
    );

    try
    {
        twoBody->Propagate();
    }
    catch (_com_error& e)
    {
        qDebug() << "Propagate failed: " << e.ErrorMessage();
        return;
    }

    // 添加宽波束传感器
    STKObjects::IAgStkObjectPtr pSensorObj = pSat;
    STKObjects::IAgSensorPtr pSen = pSensorObj->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName);
    IAgSnSimpleConicPatternPtr patternData = pSen->CommonTasks->SetPatternSimpleConic("4.0", 0.1);  //(传感器圆锥角，角分辨率-扫描精度，越小越精确)
    pSen->Graphics->Color = 0xFF000000; // 蓝色

    // 对10个用户添加凝视传感器
    // 凝视传感器0
    STKObjects::IAgStkObjectPtr pSensorObj_0 = pSat;
    STKObjects::IAgSensorPtr pSen_0 = pSensorObj_0->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName0);
    IAgSnSimpleConicPatternPtr patternData_0 = pSen_0->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_0->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_0->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/0");
    // 凝视传感器1
    STKObjects::IAgStkObjectPtr pSensorObj_1 = pSat;
    STKObjects::IAgSensorPtr pSen_1 = pSensorObj_1->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName1);
    IAgSnSimpleConicPatternPtr patternData_1 = pSen_1->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_1->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_1->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/1"); // 凝视传感器0
    // 凝视传感器2
    STKObjects::IAgStkObjectPtr pSensorObj_2 = pSat;
    STKObjects::IAgSensorPtr pSen_2 = pSensorObj_2->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName2);
    IAgSnSimpleConicPatternPtr patternData_2 = pSen_2->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_2->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_2->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/2");
    // 凝视传感器3
    STKObjects::IAgStkObjectPtr pSensorObj_3 = pSat;
    STKObjects::IAgSensorPtr pSen_3 = pSensorObj_3->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName3);
    IAgSnSimpleConicPatternPtr patternData_3 = pSen_3->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_3->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_3->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/3");
    // 凝视传感器4
    STKObjects::IAgStkObjectPtr pSensorObj_4 = pSat;
    STKObjects::IAgSensorPtr pSen_4 = pSensorObj_4->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName4);
    IAgSnSimpleConicPatternPtr patternData_4 = pSen_4->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_4->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_4->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/4");
    // 凝视传感器5
    STKObjects::IAgStkObjectPtr pSensorObj_5 = pSat;
    STKObjects::IAgSensorPtr pSen_5 = pSensorObj_5->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName5);
    IAgSnSimpleConicPatternPtr patternData_5 = pSen_5->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_5->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_5->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/5");
    // 凝视传感器6
    STKObjects::IAgStkObjectPtr pSensorObj_6 = pSat;
    STKObjects::IAgSensorPtr pSen_6 = pSensorObj_6->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName6);
    IAgSnSimpleConicPatternPtr patternData_6 = pSen_6->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_6->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_6->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/6");
    // 凝视传感器7
    STKObjects::IAgStkObjectPtr pSensorObj_7 = pSat;
    STKObjects::IAgSensorPtr pSen_7 = pSensorObj_7->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName7);
    IAgSnSimpleConicPatternPtr patternData_7 = pSen_7->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_7->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_7->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/7");
    // 凝视传感器8
    STKObjects::IAgStkObjectPtr pSensorObj_8 = pSat;
    STKObjects::IAgSensorPtr pSen_8 = pSensorObj_8->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName8);
    IAgSnSimpleConicPatternPtr patternData_8 = pSen_8->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_8->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_8->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/8");
    // 凝视传感器9
    STKObjects::IAgStkObjectPtr pSensorObj_9 = pSat;
    STKObjects::IAgSensorPtr pSen_9 = pSensorObj_9->GetChildren()->New(AgESTKObjectType::eSensor, bstrSenName9);
    IAgSnSimpleConicPatternPtr patternData_9 = pSen_9->CommonTasks->SetPatternSimpleConic("0.5", 0.1);
    pSen_9->Graphics->Color = 0x000000ff; // 传感器颜色设置为红色
    pSen_9->CommonTasks->SetPointingTargetedTracking(AgETrackModeType::eTrackModeTranspond, AgEBoresightType::eBoresightRotate, "*/Facility/9");


    // 关闭宽波束显示
    QString qstrSatName = QString::fromStdWString(static_cast<wchar_t*>(bstrSatName));
    QString qstrSenName = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName));
    SenserioOnOff(qstrSatName, qstrSenName, false);

    // 关闭显示凝视显示
    // bstr_t 转 QString ->QString::fromStdWString
    // bstr_t 对象具有一个 operator wchar_t*() 方法，允许它被隐式转换为 wchar_t* ,再通过std::wstring转为QString
    QString qstrSenName0 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName0));
    QString qstrSenName1 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName1));
    QString qstrSenName2 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName2));
    QString qstrSenName3 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName3));
    QString qstrSenName4 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName4));
    QString qstrSenName5 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName5));
    QString qstrSenName6 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName6));
    QString qstrSenName7 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName7));
    QString qstrSenName8 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName8));
    QString qstrSenName9 = QString::fromStdWString(static_cast<wchar_t*>(bstrSenName9));
    SenserioOnOff(qstrSatName, qstrSenName0, false);
    SenserioOnOff(qstrSatName, qstrSenName1, false);
    SenserioOnOff(qstrSatName, qstrSenName2, false);
    SenserioOnOff(qstrSatName, qstrSenName3, false);
    SenserioOnOff(qstrSatName, qstrSenName4, false);
    SenserioOnOff(qstrSatName, qstrSenName5, false);
    SenserioOnOff(qstrSatName, qstrSenName6, false);
    SenserioOnOff(qstrSatName, qstrSenName7, false);
    SenserioOnOff(qstrSatName, qstrSenName8, false);
    SenserioOnOff(qstrSatName, qstrSenName9, false);
}

void QSTKEarth::CreatSpecificSenserio()
{
    AddFacility(1.1620, 36.002);
    AddFacility(-3.718, 35.167);
    AddFacility(11.426, 29.536);
    AddFacility(-12.966, 27.058);
    AddFacility(3.429, 22.515);
    AddFacility(10.923, 43.208);
    AddFacility(-9.942, 19.360);
    AddFacility(-15.871, 41.528);
    AddFacility(-5.031, 51.077);
    AddFacility(-4.046, 45.533);
    //AddFacility(-16.005, 124.871);
//    AddFacility(-16.594, 113.500);
//    AddFacility(2.169, 122.001);
//    AddFacility(-12.257, 121.019);
//    AddFacility(12.279, 120.551);
//    AddFacility(-4.212, 114.458);
//    AddFacility(6.332, 105.277);
//    AddFacility(-13.525, 119.390);
//    AddFacility(12.827, 111.629);
//    AddFacility(-20.284, 115.798);
//    AddFacility(-8.651, 97.356);
    //AddFacility(10.066, 137.031);

    AddSpecificSatellite();
}



