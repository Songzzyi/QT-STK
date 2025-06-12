#pragma once
#include "QT_STK.h"
#pragma execution_character_set("utf-8")

/******************************** UDP Reciver ********************************/

bool isPreviousTimeAllZero(const u8 previousTime[13]);
// 读取待处理数据包
void QT_STK::readPendingDatagrams()
{
    // 检查是否有未等待处理的数据报文，有->进入while循环
    while (udpListener->hasPendingDatagrams())   // hasPendingDatagrams() -> 是否有未等待处理的数据报文
    {
        // 调整datagram对象大小，确保其能够容纳整个数据报文
        QByteArray datagram;
        datagram.resize(udpListener->pendingDatagramSize()); // pendingDatagramSize() -> 返回下一个等待处理的数据报文的大小

        //readDatagram读取报文，同时获取发送者的地址sender，端口senderPort
        QHostAddress sender;
        quint16 senderPort;
        udpListener->readDatagram(datagram.data(), datagram.size(), & sender, & senderPort);
        QString senderString = sender.toString();

        // 检查是否有前缀“::ffff:”，并去除。（这是IPv6相关部分，现在用的Ipv4，故无需显示）
        if (senderString.startsWith("::ffff:"))
        {
            senderString = senderString.mid(7);
        }

        QString msg = QString("消息来自: %1:%2: %3").arg(senderString).arg(senderPort).arg(QString(datagram.toHex()));
        //msgLog(msg);
        //qDebug() << "msg"<<endl;
        //qDebug() << "Received datagram of size:" << datagram.size();

        // 首先包大小应至少大于 MESSAGE_HEAD_EXTER 和 MESSAGE_TAIL_EXTER
        if(static_cast<size_t>(datagram.size()) > sizeof (MESSAGE_HEAD_EXTER) + sizeof (MESSAGE_TAIL_EXTER))
        {
            // rawData 用于定位数据第一字节
            u8* rawData = reinterpret_cast<u8*>(datagram.data());
            MESSAGE_HEAD_EXTER* head = reinterpret_cast<MESSAGE_HEAD_EXTER*>(rawData);
            //qDebug() << "Head ID:" << head->HEAD_ID << ", QT IDENTITY:" << head->QT_IDENTITY;

            // 判断包头以及是否是给Qt的包类型
            if(head->HEAD_ID == HEAD_ID_MATCH && head->QT_IDENTITY == QT_IDENTITY_MATCH)
            {
                u16 len_exter = head->LEN_EXTER;
                //qDebug() << "Length of external message:" << len_exter;

                if(static_cast<size_t>(datagram.size()) >= sizeof(MESSAGE_HEAD_EXTER) + len_exter + sizeof(MESSAGE_TAIL_EXTER))
                {
                    MESSAGE_TAIL_EXTER* tail = reinterpret_cast<MESSAGE_TAIL_EXTER*>(rawData + sizeof(MESSAGE_HEAD_EXTER) + len_exter);
                    //qDebug() << "Tail ID:" << tail->TAIL_ID;
                    if(tail->TAIL_ID == TAIL_ID_MATCH)
                    {
                        // 至此说明包完全匹配上 -> 进一步送入对应的包处理器中处理
                        //MESSAGE_INTER* dataInter = reinterpret_cast<MESSAGE_INTER*>(rawData + sizeof (MESSAGE_HEAD_EXTER)); // 下面方法等效
                        MESSAGE_INTER* dataInter = reinterpret_cast<MESSAGE_INTER*>(head->DATA_EXTER);
                        //qDebug() << "Data Type:" << dataInter->DATA_TYPE << ", Priority:" << dataInter->PRIORITY << ", Length of internal message:" << dataInter->LEN_INTER;
                        // 收到打印包
                        if(dataInter->DATA_TYPE == TYPE_StringMsg_MATCH)
                        {
                            // StringMessage
                            QString stringData = QString::fromUtf8(reinterpret_cast<char*>(dataInter->DATA_INTER));
                            msgLog(stringData);
                        }
                        // 收到用户离线包
                        else if(dataInter->DATA_TYPE == TYPE_UEDISCON)
                        {
                            // UEDISCON
                            UEDISCON* UEdisconnectData = reinterpret_cast<UEDISCON*>(dataInter->DATA_INTER);
                            u8 sat_id = UEdisconnectData->SAT_ID;
                            u16 beam_num = UEdisconnectData->Beam_num;
                            u8 ue_address = UEdisconnectData->UE_ADDRESS[5];
                            msgLog(QString("卫星%1(基带板%2) - 用户%3离线").arg(sat_id).arg(beam_num).arg(ue_address));
                            QString ueName = "UE" + QString::number(ue_address);
                            twUEsStateChange(ueName, "离线");
                        }
                        else
                        {
                            // MIB /RARSP /RAREQ /TF 包处理
                            EVENT event;
                            // 基本数据（通常小）直接赋值操作 - 更快速
                            event.dataType = dataInter->DATA_TYPE;
                            event.priority = dataInter->PRIORITY;
                            qDebug() << "Current TIME: " << QString::fromUtf8(reinterpret_cast<const char*>(dataInter->TIME), 12);
                            if(isPreviousTimeAllZero(previousTime) && event.dataType != TYPE_TF)
                            {
                                event.time = 0;
                                qDebug() << "Previous TIME is all zero, setting event.time to 0.";
                                memcpy(previousTime, dataInter->TIME, 13);
                            }
                            else
                            {
                                if(event.dataType != TYPE_TF)
                                {
                                    event.time = TIME_to_EVENTtime(previousTime, dataInter->TIME);
                                    qDebug() << "Calculated event.time: " << event.time;
                                }
                            }
                            // 方法一：QByteArray 的构造函数将自动处理内存分配和数据复制
                            //event.data = QByteArray(reinterpret_cast<char*>(dataInter->DATA_INTER), dataInter->LEN_INTER);
                            // 方法二：memcpy - 待测试是否可行
                            // 分配足够的内存来存储有效数据，并复制数据
                            event.data.resize(dataInter->LEN_INTER);
                            std::memcpy(event.data.data(), dataInter->DATA_INTER, dataInter->LEN_INTER);

                            // 特别注意：
                            // 下面QByteArray::fromRawData()方法仅采用数据引用，而不是数据复制，原有的QByteArray生命周期内被修改的话
                            // 那么通过 fromRawData() 创建的 QByteArray 将指向不正确或不再存在的内存区域！！
                            // u8* dataInter->DATA_INTER 转为 char*, QByteArray::fromRawData函数：从给定的原始数据创建一个QByteArray 对象
                            //event.data = QByteArray::fromRawData(reinterpret_cast<char*>(dataInter->DATA_INTER),dataInter->LEN_INTER);
                            // 创建特定message_type对应的派生类对象，返回PacketHandler类型的指针，指向并指向这个派生类对象
                            PacketHandler* packetHandler = PacketHandlerFactory::createHandler(event.dataType);
                            if(packetHandler)
                            {
                                // qDebug() << "Handling packet with data type:" << event.dataType;
                                packetHandler->handlePacket(event);
                                delete packetHandler;
                            }
                            else
                            {
                                qDebug() << "No handler found for data type:" << event.dataType;
                            }
                        }
                    }
                }
            }
        }
    }
}

// 检查 previousTime 是否全零
bool isPreviousTimeAllZero(const u8 previousTime[13])
{
    for(int i = 0; i < 13; i++)
    {
        if(previousTime[i] != 0)
        {
            return false;
        }
    }
    return true;
}

// 处理接收到数据包中的时间戳字符串 返回和前一个包的时间戳差（ms）
u32 QT_STK::TIME_to_EVENTtime(u8* previousTimePtr, const u8 currentTime[13])
{
    QString PreTimeString = QString::fromUtf8(reinterpret_cast<const char*>(previousTimePtr), 12);
    QString CurTimeString = QString::fromUtf8(reinterpret_cast<const char*>(currentTime), 12);

    int preHours = PreTimeString.mid(0, 2).toInt();
    int preMinutes = PreTimeString.mid(3, 2).toInt();
    int preSeconds = PreTimeString.mid(6, 2).toInt();
    int preMilliseconds = PreTimeString.mid(9, 3).toInt();
    int preMillisecondTime = preMilliseconds + preSeconds * 1000 + preMinutes * 60 * 1000 + preHours * 60 * 60 * 1000;

    int curHours = CurTimeString.mid(0, 2).toInt();
    int curMinutes = CurTimeString.mid(3, 2).toInt();
    int curSeconds = CurTimeString.mid(6, 2).toInt();
    int curMilliseconds = CurTimeString.mid(9, 3).toInt();
    int curMillisecondTime = curMilliseconds + curSeconds * 1000 + curMinutes * 60 * 1000 + curHours * 60 * 60 * 1000;

    // 时间差
    int diffTime = curMillisecondTime - preMillisecondTime;
    if(diffTime < 0)
    {
        // 超过一天时间 则加上一天的毫秒数
        diffTime += 24 * 60 * 60 * 1000;
    }
    qDebug() << "1 - Updating previousTimePtr, before memcpy: " << QString::fromUtf8(reinterpret_cast<const char*>(previousTimePtr), 12);
    memcpy(previousTimePtr, currentTime, 13);
    qDebug() << "2 - Updated previousTimePtr, after memcpy: " << QString::fromUtf8(reinterpret_cast<const char*>(previousTimePtr), 12);
    qDebug() << "3 - diffTime: " << diffTime;
    return static_cast<u32>(diffTime);
}

/******************************** UDP Sender ********************************/
// tableWidget 控件输出 时间 与 事件
void QT_STK::msgLog(const QString& msg)
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss"); // 获取当前时间

    ui->tableWidget->insertRow(0);   // 插入新行
    ui->tableWidget->setRowHeight(0, 30); // 设置新行的行高

    // 创建时间和事件的QTableWidgetItem
    QTableWidgetItem* timeItem = new QTableWidgetItem(currentTime);
    QTableWidgetItem* msgItem = new QTableWidgetItem(msg);

    // 设置文本居中对齐
    timeItem->setTextAlignment(Qt::AlignCenter);
    msgItem->setTextAlignment(Qt::AlignCenter);
    // Qt::AlignCenter 表示单元格内水平和居中对齐
    // Qt::AlignLeft | Qt::AlignVCenter 左对齐和垂直居中对齐
    // 设置单元格为不可编辑状态
    timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
    msgItem->setFlags(msgItem->flags() & ~Qt::ItemIsEditable);
    //msgItem->setFlags(msgItem->flags() | Qt::ItemIsEditable); // 设置 msgItem 为可编辑状态
    // 将时间和事件添加到tableWidget的新行中
    ui->tableWidget->setItem(0, 0, timeItem);
    ui->tableWidget->setItem(0, 1, msgItem);

    // 自动调整行高以适应内容
    ui->tableWidget->resizeRowsToContents();

    // 更新行号
    int rowCount = ui->tableWidget->rowCount(); // 获取当前行数
    for (int i = 0; i <= rowCount; ++i)
    {
        QTableWidgetItem* rowNumberItem = new QTableWidgetItem(QString::number(rowCount - i));
        rowNumberItem->setTextAlignment(Qt::AlignCenter);
        rowNumberItem->setFlags(rowNumberItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setVerticalHeaderItem(i, rowNumberItem);   // 将行号设置到垂直标题
    }

    // 获取当前的滚动位置
    QModelIndex currentTopIndex = ui->tableWidget->indexAt(ui->tableWidget->rect().topLeft());
    // 打印 currentTopIndex 信息
    //QString indexInfo = QString("Row: %1, Column: %2").arg(currentTopIndex.row()).arg(currentTopIndex.column());
    //QMessageBox::information(this, "Current Top Index", indexInfo);
    // 是否自动滚动刷新
    if(autoScrollEnabled)
    {
        // 自动滚动到第一行
        ui->tableWidget->scrollToItem(ui->tableWidget->item(0, 0), QAbstractItemView::PositionAtTop);
    }
    else
    {
        // 获取插入新行后的第一个可见行的索引
        QModelIndex newTopIndex = ui->tableWidget->model()->index(currentTopIndex.row() + 1, 0);    // 插入新行，旧行行号则会加1，故这里要加1
        // 恢复到插入新行前的位置
        ui->tableWidget->scrollTo(newTopIndex, QAbstractItemView::PositionAtTop);
    }
}

// 发生连接错误时，记录错误信息并显示在日志中
void QT_STK::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString errorString;
    switch (socketError)
    {
        case QAbstractSocket::RemoteHostClosedError:
            errorString = "Remote Host Closed Error";
            break;
        case QAbstractSocket::HostNotFoundError:
            errorString = "Host Not Found Error";
            break;
        case QAbstractSocket::ConnectionRefusedError:
            errorString = "Connection Refused Error";
            break;
        default:
            errorString = "Unknown Error";
            break;
    }
    msgLog(QString("UDP连接错误,错误号:%1，错误信息:%2").arg(socketError).arg(errorString));
}

// 将发送的数据是为十六进制数值，并按二进制发送出去
void QT_STK::udpDataSend()
{
    QString StringData = ui->textEdit->toPlainText();
    QByteArray dataToSend;

    // 选择发送模式
    if(ui->rb_AsciiMode->isChecked())
    {
        // ASCII 模式 - 计算机将每个字符视为ASCII字符串，将对应ASCII码发送出去
        dataToSend = StringData.toUtf8(); // QString 转 QByteArray
    }
    else if(ui->rb_HexMode->isChecked())
    {
        // HEX 模式 - 将发送的数据视为十六进制数值，并按二进制发送出去
        bool ok;
        for (int i = 0; i < StringData.length(); i += 2)
        {
            // 从 stringData 的第 i 个字符开始，提取长度为 2 的子字符串
            QString byteString = StringData.mid(i, 2); // 获取每两个字符
            // toUInt 将byteString解析为无符号整数（unsigned int），并按照16进制解析
            char byte = byteString.toUInt( & ok, 16); // 将其转换为一个字节
            if (ok)
            {
                dataToSend.append(byte);
            }
            else
            {
                msgLog("无效的十六进制数值！");
                return;
            }
        }
    }

    // 发送数据
    if (u_udp && u_udp->isOpen())
    {
        u_udp->write(dataToSend);
    }
    else
    {
        msgLog("请连接后再次发送");
    }

    /***************** 发送方式1 *****************/
//    QString StringData = ui->textEdit->toPlainText(); // textEdit 获取输入的数据
//    qDebug() << "StringData:" << StringData << endl;
//    QByteArray ByteData = StringData.toUtf8(); // 将 QString 转换为 UTF-8编码的 QByteArray
//    qDebug() << "StringData.toUtf8():" << ByteData << endl;
//    QByteArray dataToSend = ByteData.toHex();
//    qDebug() << "ByteData.toHex():" << dataToSend << endl;

//    // 发送数据
//    if (u_udp && u_udp->isOpen()) {
//        u_udp->write(dataToSend);
//    } else {
//        msgLog("请连接后再次发送");
//    }

    /***************** 发送方式2 *****************/
//    QString stringData = ui->textEdit->toPlainText(); // textEdit 获取输入的数据
//    QByteArray byteData;

//    // 转换为16进制的 QByteArray
//    bool ok;
//    for (int i = 0; i < stringData.length(); i += 2) {
//        QString byteString = stringData.mid(i, 2); // 获取每两个字符
//        char byte = byteString.toUInt(&ok, 16); // 将其转换为一个字节
//        if (ok) {
//            byteData.append(byte);
//        } else {
//            msgLog("Invalid hex data");
//            return;
//        }
//    }
//    // 发送数据
//    if (u_udp && u_udp->isOpen()) {
//        u_udp->write(byteData);
//    } else {
//        msgLog("请连接后再次发送");
//    }
}

// 无需绑定本机端口，系统会自动为这个套接字分配一个临时端口
void QT_STK::on_pb_connect_clicked()
{
    QString ip = ui->LineEdit_ipv4->text();
    unsigned short port = ui->LineEdit_port->text().toInt();

    // 创建通信的套接字对象
    u_udp = new QUdpSocket(this);

    // 直接设置目标及其端口号
    u_udp->connectToHost(QHostAddress(ip), port);
    msgLog("尝试连接到 " + ip + ":" + QString::number(port));

    ui->pb_connect->setEnabled(false);
    ui->pb_disconnect->setEnabled(true);
    ui->pb_udpsend->setEnabled(true);
    ui->LineEdit_ipv4->setEnabled(false);
    ui->LineEdit_port->setEnabled(false);
    // 检测服务器是否回复了数据
    connect(u_udp, & QUdpSocket::readyRead, this, & QT_STK::readPendingDatagrams);

    connect(u_udp, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
}

void QT_STK::on_pb_disconnect_clicked()
{
    if (u_udp)
    {
        u_udp->close();
        delete u_udp;
        u_udp = nullptr;
    }
    ui->pb_connect->setText("连接");
    ui->pb_connect->setEnabled(true);
    ui->pb_disconnect->setEnabled(false);
    ui->pb_udpsend->setEnabled(false);
    ui->LineEdit_ipv4->setEnabled(true);
    ui->LineEdit_port->setEnabled(true);
}

void QT_STK::on_pb_udpsend_clicked()
{
    udpDataSend();

//    // 所有用户离线
//    if(ui->cb_cmd->currentText() == "卫星卸载在线用户")
//    {
//        twUEsStateOffline_All();
//    }
}


