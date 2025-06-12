#include "QT_STK.h"
#include <QDebug>
#include <windows.h> // 需要包含这个头文件来使用 Sleep 函数
#include <QUuid>
#include <QMessageBox>

#pragma execution_character_set("utf-8")

u8 QT_STK::previousTime[13] = {0};

QT_STK::QT_STK(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::QT_STK)
    , udpListener(nullptr) // 构造函数初始化udpListener为nullptr，延迟创建udpListener，为的是在新线程中创建并操作
    , autoScrollEnabled(true)   // 默认启用自动滚动
    , TFShowTimer(new QTimer(this))
    , EVENTShowTimer(new QTimer(this))
{
    ui->setupUi(this);
    // 初始化按键信息
    ui->pb_init->setEnabled(true);
    ui->pb_del->setEnabled(false);
    ui->pb_addFac->setEnabled(false);
    ui->pb_addSat->setEnabled(false);
    // 动画持续时间 - 默认2s
    ui->Animation_duration->setText("2.0");
    animationDuration = 2000;

    /**************** 定时器与TF显示速率 初始化部分 ****************/
    connect(TFShowTimer, & QTimer::timeout, this, & QT_STK::processTFQueue);
    ui->cb_TFShowTime->setCurrentIndex(1);  // 默认显示第2个条目 2s
    TFShowTimer->start(2000); // 对应默认条目：2s（2000ms）触发一次定时器
    // 设置QComboBox当前项文本居中显示
    QLineEdit* lineEdit_1 = new QLineEdit();
    lineEdit_1->setAlignment(Qt::AlignCenter);
    ui->cb_TFShowTime->setLineEdit(lineEdit_1);
    lineEdit_1->setEnabled(false);
    ui->cb_TFShowTime->setItemDelegate(new CenterAlignDelegate(ui->cb_TFShowTime)); // 设置下拉菜单居中代理

    /**************** 事件显示速率 初始化部分 ****************/
    ui->cb_EventPriority->setCurrentIndex(0);  // 默认显示第一个条目 “最低优先级” cb_EventShowTime
    ui->cb_EventShowTime->setCurrentIndex(0);  // 默认选择 “自动”
    adjustEventTimer();
    connect(EVENTShowTimer, & QTimer::timeout, this, & QT_STK::processEventQueue);

    // 设置QComboBox当前项文本居中显示
    QLineEdit* lineEdit_2 = new QLineEdit();    // 单行文本输入控件QLineEdit
    lineEdit_2->setAlignment(Qt::AlignCenter);  // 设置QLineEdit居中
    lineEdit_2->setEnabled(false);
    ui->cb_EventPriority->setLineEdit(lineEdit_2);  // 将自定义的QLineEdit设置为ComboBox的lineEdit
    ui->cb_EventPriority->setItemDelegate(new CenterAlignDelegate(ui->cb_EventPriority)); // 设置下拉菜单居中代理
    QLineEdit* lineEdit_3 = new QLineEdit();    // 单行文本输入控件QLineEdit
    lineEdit_3->setAlignment(Qt::AlignCenter);  // 设置QLineEdit居中
    lineEdit_3->setEnabled(false);
    ui->cb_EventShowTime->setLineEdit(lineEdit_3);  // 将自定义的QLineEdit设置为ComboBox的lineEdit
    ui->cb_EventShowTime->setItemDelegate(new CenterAlignDelegate(ui->cb_EventShowTime)); // 设置下拉菜单居中代理

    /************** tableWidget 日志打印部分 **************/
    // 设置表头
    ui->tableWidget->setColumnCount(2); // 设置列数为2
    QStringList headersList;
    headersList << "时间";
    headersList << "事件";
    ui->tableWidget->setHorizontalHeaderLabels(headersList);
//    // 获取水平表头对象
//    QHeaderView* header = ui->tableWidget->horizontalHeader();
//    // 设置水平表头文字居中
//    header->setDefaultAlignment(Qt::AlignCenter);
//    // 启用水平和垂直滚动条
//    ui->tableWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
//    ui->tableWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // 设置单元格内容为多行显示
    ui->tableWidget->setWordWrap(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    // 设置初始列宽
    int tableWidth = ui->tableWidget->width();
    ui->tableWidget->setColumnWidth(0, 200);
    ui->tableWidget->setColumnWidth(1, tableWidth - 200);
    // 选择模式为‘行’选择
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    /************** twUEs UE用户接入显示 **************/
    ui->twUEs->setColumnCount(2);
    QStringList twUEsheaders;
    twUEsheaders << "用户";
    twUEsheaders << "接入状态";
    ui->twUEs->setHorizontalHeaderLabels(twUEsheaders);
    int twUEsWidth = ui->twUEs->width();
    ui->twUEs->setColumnWidth(0, twUEsWidth / 2);
    ui->twUEs->setColumnWidth(1, twUEsWidth / 2);
    ui->twUEs->setSelectionBehavior(QAbstractItemView::SelectRows);

    /************** SocketUDP 通信部分 **************/
    ui->pb_disconnect->setEnabled(false);
    ui->pb_udpsend->setEnabled(false);
    // 创建并启动本地监听线程
    udpListenerThread = new QThread(this);
    connect(udpListenerThread, & QThread::started, this, & QT_STK::setupUdpListener);
    udpListenerThread->start();
    // 控制台默认配置
    ui->cb_cmd->setCurrentIndex(0);
    ui->textEdit->setText("2277060000030000000023");
    ui->LineEdit_ipv4->setText("192.188.");
    ui->rb_HexMode->setChecked(true);
}

//QT_STK& QT_STK::getInstance()
//{
//    static QT_STK instance;
//    return instance;
//}

void QT_STK::setupUdpListener()
{
    udpListener = new QUdpSocket();
    // udpListener 有数据可读时，发出readyRead信号，进而调用readPendingDatagrams
    connect(udpListener, & QUdpSocket::readyRead, this, & QT_STK::readPendingDatagrams);
    if (!udpListener->bind(QHostAddress::Any, 7788))
    {
        msgLog("本地UDP绑定失败: 端口可能被占用");
    }
    else
    {
        msgLog("本地UDP绑定成功: 绑定到本地端口 7788");
    }
}

QT_STK::~QT_STK()
{
    // 停止并等待udpListener线程完成
    if (udpListenerThread->isRunning())
    {
        udpListenerThread->quit();  // 请求线程停止运行
        udpListenerThread->wait();  // 等待线程完成
    }

    delete udpListener;
    delete udpListenerThread;
    delete ui;
}

void QT_STK::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    // 获取tableWidget的宽度
    int tableWidth = ui->tableWidget->width();
    // 设置列宽
    ui->tableWidget->setColumnWidth(0, 200); // 第1列宽度设为200
    ui->tableWidget->setColumnWidth(1, tableWidth - 200); // 第2列宽度设为tableWidth - 200
}

void QT_STK::on_pb_init_clicked()
{
    ui->pb_del->setEnabled(true);
    ui->pb_addFac->setEnabled(true);
    ui->pb_addSat->setEnabled(true);
    // 获取 QSTKEarth 类的单例实例
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->NewScenario();
    m_stkEarth->CreatSpecificSenserio();
    msgLog("创建场景");
}

void QT_STK::on_pb_del_clicked()
{
    ui->pb_del->setEnabled(false);
    ui->pb_addFac->setEnabled(false);
    ui->pb_addSat->setEnabled(false);
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->UnloadStkScence();
    msgLog("卸载场景");
}

void QT_STK::on_pb_addSat_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->AddRandomSatellite();
    //m_stkEarth->AddSatelliteWithSensor();
    //m_stkEarth->AddSpecificSatellite();
}

void QT_STK::on_pb_addFac_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->AddRamdonFacility();
    msgLog("随机添加地面站");
}

void QT_STK::on_pb_forward_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->PlayForward();
    m_stkEarth->isPlayForward = true;
    ui->pb_pause->setText("暂停");
    msgLog("动画-前向播放");
}

void QT_STK::on_pb_backward_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->PlayBackward();
    m_stkEarth->isPlayForward = false;
    ui->pb_pause->setText("暂停");
    msgLog("动画-后向播放");
}

void QT_STK::on_pb_pause_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    if(ui->pb_pause->text() == "暂停")
    {
        m_stkEarth->Pause();
        ui->pb_pause->setText("继续");
    }
    else if(ui->pb_pause->text() == "继续")
    {
        if(m_stkEarth->isPlayForward)
        {
            m_stkEarth->PlayForward();
            m_stkEarth->isPlayForward = true;
        }
        else if(!m_stkEarth->isPlayForward)
        {
            m_stkEarth->PlayBackward();
            m_stkEarth->isPlayForward = false;
        }
        ui->pb_pause->setText("暂停");
    }
    msgLog("动画-暂停");
    // test

}

void QT_STK::on_pb_rewind_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->Rewind();
    msgLog("动画-复位");
}

void QT_STK::on_pb_slow_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->SlowerSTK();
    msgLog("动画-减速");
}

void QT_STK::on_pb_fast_clicked()
{
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->FasterSTK();
    msgLog("动画-加速");
}


void QT_STK::on_cb_autoScroll_stateChanged(int state)
{
    this->autoScrollEnabled = (state == Qt::Checked);
}

// 设置TF时频资源池更新速率
void QT_STK::on_cb_TFShowTime_currentIndexChanged(const QString& text)
{
    bool OK;
    int value = text.toDouble( & OK);
    if(OK)
    {
        TFShowTimer->start(value * 1000);
    }
}

// 与currentTextChanged等效
//void QT_STK::on_cb_EventShowTime_currentIndexChanged(const QString &text)
//{
//    bool OK;
//    int value = text.toDouble(&OK);
//    if(OK){
//        EVENTShowTimer->start(value * 1000);
//    }
//}


// 设置EVENT更新速率
void QT_STK::on_cb_EventShowTime_currentTextChanged(const QString& text)
{
    qDebug() << "Entered on_cb_EventShowTime_currentTextChanged with text:" << text;

    if(text == "自动")
    {
        EVENTShowTimer->stop();
        QMutexLocker locker( & EventQueueMutex);   // 加锁
        adjustEventTimer(); // 动态调整定时器时间
    }
    else
    {
        bool OK;
        double value = text.toDouble( & OK);
        //qDebug() << "Conversion result:" << OK << ", value:" << value;

        if(OK && EVENTShowTimer)
        {
            qDebug() << "Starting EVENTShowTimer with interval:" << value * 1000;
            EVENTShowTimer->start(value * 1000);
        }
        else
        {
            qDebug() << "Invalid conversion or timer not initialized.";
        }
    }
}

// 动态调整事件队列弹出间隔
void QT_STK::adjustEventTimer()
{
    qint64 dynamicInterval;
    if(!EventQueue.empty())
    {
        EVENT nextEvent = EventQueue.front();
        dynamicInterval = nextEvent.time * 200;  // 暂定200位时间放大因子

        // 限制动态的最大间隔范围 暂定: 3s <= dynamicInterval <= 30s
        if(dynamicInterval > 30000)
        {
            dynamicInterval = 30000;    // 将间隔压缩为不超过30s
        }
        else if(dynamicInterval < 3000)
        {
            dynamicInterval = 3000;     // 将间隔放大为至少3s
        }
        qDebug() << "更新定时器：" << dynamicInterval << endl;
    }
    else
    {
        // 队列为空，则设置一个默认较小的时间间隔 暂定 3s
        dynamicInterval = 3000;
        qDebug() << "队列为空，定时器：" << dynamicInterval << endl;
    }
    EVENTShowTimer->start(dynamicInterval);
}


void QT_STK::on_cb_cmd_currentTextChanged(const QString& text)
{
    if(text == "控制台测试联通命令")
    {
        ui->textEdit->setText("2277060000030000000023");
    }
    else if(text == "卫星MIB发送开启")
    {
        ui->textEdit->setText("227707000101010001000123");
    }
    else if(text == "卫星MIB发送关闭")
    {
        ui->textEdit->setText("227707000101010001000023");
    }
    else if(text == "用户MIB接收开启")
    {
        ui->textEdit->setText("227707000102000001000123");
    }
    else if(text == "用户MIB接收关闭")
    {
        ui->textEdit->setText("227707000102000001000023");
    }
    else if(text == "卫星卸载在线用户")
    {
        ui->textEdit->setText("22770A00020101000400C0A8640923");
    }
    else if(text == "用户主动离线")
    {
        ui->textEdit->setText("2277060002020800000023");
    }
    else if(text == "卫星下行业务开启接收")
    {
        ui->textEdit->setText("227707000301010001000123");
    }
    else if(text == "卫星下行业务关闭接收")
    {
        ui->textEdit->setText("227707000301010001000023");
    }
    else if(text == "用户下行业务开启接收")
    {
        ui->textEdit->setText("227707000302000001000123");
    }
    else if(text == "用户下行业务关闭接收")
    {
        ui->textEdit->setText("227707000302000001000023");
    }
    else if(text == "卫星指定用户星内切换")
    {
        ui->textEdit->setText("22770A00040101000400C0A8640A23");
    }
    else if(text == "卫星负载均衡开启")
    {
        ui->textEdit->setText("227707000501010001000123");
    }
    else if(text == "卫星负载均衡关闭")
    {
        ui->textEdit->setText("227707000501010001000023");
    }
    else if(text == "用户星间切换")
    {
        ui->textEdit->setText("22770A000402000004000196000223");
    }
    else if(text == "卫星随机接入开启")
    {
        ui->textEdit->setText("227707000601010001000123");
    }
    else if(text == "卫星随机接入关闭")
    {
        ui->textEdit->setText("227707000601010001000023");
    }
    else if(text == "自定义数据")
    {
        ui->textEdit->setText("");
    }
}

void QT_STK::on_Animation_duration_textChanged(const QString& text)
{
    if(!text.isEmpty())
    {
        bool ok;
        // 注意：这里用的toInt将会导致ok为false
        // toInt不支持小数点，如果 text 中有小数，会导致解析失败
        double value = text.toDouble( & ok);
        if(ok && value > 0) // 注意：value = 0 时不更新
            this->animationDuration = static_cast<int>(value * 1000);
        else
            QMessageBox::warning(this, "警告", "动作持续时间应为正值！");
    }
}

// 点击清空初始化 日志输出
void QT_STK::on_pb_clear_log_clicked()
{
    ui->tableWidget->clearContents();  // 清空内容，保留表头
    ui->tableWidget->setRowCount(0);   // 设置行数为0，清空所有行
}
