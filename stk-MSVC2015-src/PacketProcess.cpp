#include "QT_STK.h"
#pragma execution_character_set("utf-8")    // 设置源代码文件的默认字符集为UTF-8

// 全局变量
std::queue<TF_MESSAGE> TFmessageQueue;
QMutex TFqueueMutex;  // 互斥锁---防止多个线程同时访问共享资源（队列TFmessageQueue）（1）接收线程会在队列压入数据（2）主线程会弹出队列数据
std::queue<EVENT> EventQueue;
QMutex EventQueueMutex;

// 定时弹出TF队列
void QT_STK::processTFQueue()
{
    //qDebug()<<"processTFQueue";
    // 对队列加互斥锁
    QMutexLocker locker( & TFqueueMutex);

    if(!TFmessageQueue.empty())
    {
        TF_MESSAGE message = TFmessageQueue.front();
        TFmessageQueue.pop();
        qDebug() << "TFMessage popped. Current size:" << TFmessageQueue.size();

        // 处理弹出队列头，更新UI资源池
        u8 ( * data)[10][4] = reinterpret_cast<u8 ( * )[10][4]>(message.DATA); // data 是指向二维数组u8[10][4]的指针
        // data[i] 相当于指向第i个u8[10][4]数组
        for(int i = 0; i < 8; i++)
        {
            for(int j = 0; j < 10; j++)
            {
                for(int k = 0; k < 4; k++)
                {
                    // 构建按键名称
                    QString button_name = QString("pb_%1_%2_%3").arg(i).arg(j).arg(k);
                    // findChild<*>():用于查找子对象。找到了则指向这一对象，没找到则返回nullptr
                    QPushButton* button = findChild<QPushButton*>(button_name);
                    // 等于1表示资源被占用
                    if (button)
                    {
                        if (data[i][j][k] == 1)
                        {
                            button->setStyleSheet(QString("background-color: %1;").arg(OccupiedChannel_color));
                        }
                        else if (data[i][j][k] == 0)
                        {
                            if(k == 0)
                            {
                                // 第一列为控制信道
                                button->setStyleSheet(QString("background-color: %1;").arg(ControlChannelFree_color));
                            }
                            else
                            {
                                button->setStyleSheet(QString("background-color: %1;").arg(ServiceChannelFree_color));
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        qDebug() << "TFmessageQueue is empty.";
    }
}


// 对应包处理器 - 定时弹出事件队列并执行相应操作和动画
void QT_STK::processEventQueue()
{
    //qDebug()<<"processEventQueue";
    // 加锁
    QMutexLocker locker( & EventQueueMutex);
    if(!EventQueue.empty())
    {
        EVENT selectedEvent;
        bool eventFound = false;    // 用于判断是否找到对应优先度的事件

        // 寻找符合当前优先度的下一个事件, 过程中不满足的事件直接丢弃，直到找到符合优先级事件
        while(!EventQueue.empty())
        {
            EVENT curEvent = EventQueue.front();
            EventQueue.pop();
            qDebug() << "EventQueue pop";
            qDebug() << "Time:" << curEvent.time;

            // 优先级小于qcombox中所选，则丢弃（注意：currentIndex从0开始，但是curEvent.priority为0x01 - 0x05）
            if(!eventFound && curEvent.priority >= (ui->cb_EventPriority->currentIndex() + 1))
            {
                eventFound = true;
                selectedEvent = curEvent;
                break;
                qDebug() << "eventFound = true, Current size:" << EventQueue.size();
            }
        }

        // 对事件包二次分流并处理
        if(eventFound)
        {
            switch(selectedEvent.dataType)
            {
                case TYPE_MIB:
                    MIBHandler(selectedEvent);
                    break;
                case TYPE_RAREQ:
                    RAREQHandler(selectedEvent);
                    break;
                case TYPE_RARSP:
                    RARSPHandler(selectedEvent);
                    break;
                default:
                    break;
            }
        }
    }
    else
    {
        qDebug() << "EventQueue is empty.";
    }

    if(ui->cb_EventShowTime->currentText() == "自动")
    {
        adjustEventTimer();
    }
}

// 事件包处理函数
// Handler 动画显示函数，LOG 事件打印输出函数
void QT_STK::MIBHandler(EVENT& event)
{
    // 为MIB事件随机产生一个uuid
    QString eventID = QUuid::createUuid().toString();
    currentMIBEventId = eventID;

    // 方法1：指针转换
    // event.data.data() 获取QByteArray指针再进行类型转换
    MIB* mibptr = reinterpret_cast<MIB*>(event.data.data());
    u8 sat_id = mibptr->SAT_ID;
    msgLog(QString("MIB - 卫星%1广播MIB包").arg(sat_id));
    // 方法2：内存拷贝实现
//    MIB mib;
//    memcpy(&mib,event.data.data(),sizeof (MIB));
//    u8 sat_id = mib.SAT_ID;
//    // qDebug() << "Raw data:" << event.data.toHex();  // 打印原始数据
//    msgLog(QString("MIB - 卫星%1广播MIB包").arg(sat_id));

    // 动画
    // 打开宽波束照射
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->SenserioOnOff("Sat1", "SensorWideBeam", true);

    // ### 待整理
    // QTimer::singleShot - 用来设置一个一次性定时器，在指定的时间后执行一个回调函数
    // 设置定时器，在一定时间后关闭宽波束照射

    QTimer::singleShot(this->animationDuration, [this, m_stkEarth, eventID]()
    {
        if(this->currentMIBEventId == eventID)
        {
            m_stkEarth->SenserioOnOff("Sat1", "SensorWideBeam", false);
        }
    });
}

void QT_STK::RAREQHandler(EVENT& event)
{
    // 为RAREQ事件随机产生一个uuid
    QString eventID = QUuid::createUuid().toString();
    //currentRAREQEventId = eventID;

    RAREQ* rareq = reinterpret_cast<RAREQ*>(event.data.data());
    u8 sat_id = rareq->SAT_ID;
    u8 ue_address[6];
    // 更新用户状态
    QString ueName = "UE" + QString::number(rareq->UE_ADDRESS[5]);
    twUEsStateChange(ueName, "请求接入");

    // std::copy : C++ 标准库中的算法，适用于容器和迭代器，推荐用于一般的容器和范围复制
    // memcpy    : C 标准库函数，字节级别的内存复制，适用于原始内存块的复制，推荐用于性能关键的场景或需要复制原始内存块时
    std::copy(std::begin(rareq->UE_ADDRESS), std::end(rareq->UE_ADDRESS), std::begin(ue_address));
    //memcpy(ue_address,rareq->UE_ADDRESS,sizeof (ue_address));

    msgLog(QString("RAREQ - 随机接入请求:用户%1 -> 卫星%2").arg(ue_address[5]).arg(sat_id));
    QString ue_addressStr = QString::number(ue_address[5]);
    QString satelliteName = "Sat" + QString::number(sat_id);
    // 动画
    // 添加一个细线 连接对应用户到卫星
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->SatelliteUEConnect(satelliteName, ue_addressStr, true);

    // 设置定时器，在2秒后断开连接
    QTimer::singleShot(this->animationDuration, [this, m_stkEarth, satelliteName, ue_addressStr, eventID]()
    {
        //if(this->currentRAREQEventId == eventID)
        //{
        m_stkEarth->SatelliteUEConnect(satelliteName, ue_addressStr, false);
        //}
    });
}

void QT_STK::RARSPHandler(EVENT& event)
{
    // 为RAREQ事件随机产生一个uuid
    QString eventID = QUuid::createUuid().toString();
    //currentRARSPEventId = eventID;

    RARSP* rarsp = reinterpret_cast<RARSP*>(event.data.data());
    u8 sat_id = rarsp->SAT_ID;
    u16 beam_num = rarsp->Beam_num;
    u8 ue_address[6];
    // 更新用户状态
    QString ueName = "UE" + QString::number(rarsp->UE_ADDRESS[5]);
    twUEsStateChange(ueName, "在线");

    std::copy(std::begin(rarsp->UE_ADDRESS), std::end(rarsp->UE_ADDRESS), std::begin(ue_address));
    msgLog(QString("RARSP - 分配跳波束:卫星%1(基带板%2) -> 用户%3").arg(sat_id).arg(beam_num).arg(ue_address[5]));

    QString ue_addressStr = QString::number(ue_address[5]);
    QString sensorName = "Sen" + ue_addressStr;
    QString satelliteName = "Sat" + QString::number(sat_id);
    // 动画
    // 卫星添加对对应用户的凝视
    QSTKEarth* m_stkEarth = & QSTKEarth::getInstance();
    m_stkEarth->SenserioOnOff(satelliteName, sensorName, true);

    // 设置定时器，在2秒后关闭凝视
    QTimer::singleShot(this->animationDuration, [this, m_stkEarth, satelliteName, sensorName, eventID]()
    {
        //if (this->currentRARSPEventId == eventID)
        //{
        m_stkEarth->SenserioOnOff(satelliteName, sensorName, false);
        //}
    });
}

// 新增用户状态/更改已存在用户状态
void QT_STK::twUEsStateChange(QString UEname, QString UEstate)
{
    int rowCount = ui->twUEs->rowCount();
    bool userExists = false;
    // 配置状态单元格
    QTableWidgetItem* UEstateItem = new QTableWidgetItem(UEstate);
    UEstateItem->setTextAlignment(Qt::AlignCenter); // 居中对齐
    UEstateItem->setFlags(UEstateItem->flags() & ~Qt::ItemIsEditable);  // 不可编辑

    // 遍历所有行，检查用户是否已经存在
    for(int row = 0; row < rowCount; row++)
    {
        QTableWidgetItem* item = ui->twUEs->item(row, 0);   // 获取当前行第一列用户名称
        if(item && item->text() == UEname)
        {
            // 用户存在，则只更新状态
            ui->twUEs->setItem(row, 1, UEstateItem);
            userExists = true;
            break;
        }
    }

    // 新增行
    if(!userExists)
    {
        QTableWidgetItem* UEnameItem = new QTableWidgetItem(UEname);
        // 单元格居中对齐
        UEnameItem->setTextAlignment(Qt::AlignCenter);
        // 单元格不可编辑
        UEnameItem->setFlags(UEnameItem->flags() & ~Qt::ItemIsEditable);
        // 插入行
        ui->twUEs->insertRow(rowCount);
        ui->twUEs->setRowHeight(rowCount, 20);
        ui->twUEs->setItem(rowCount, 0, UEnameItem);
        ui->twUEs->setItem(rowCount, 1, UEstateItem);
    }
}

void QT_STK::twUEsStateOffline_All()
{
    int rowCount = ui->twUEs->rowCount();
    for(int row = 0; row < rowCount; row++)
    {
        QTableWidgetItem* item = ui->twUEs->item(row, 1);
        item->setText("离线");
    }
}











