#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <QString>
#include <QDateTime>    // 获取当前时间

class TimeManager{
public:
    TimeManager();
    ~TimeManager();
    QString getCurrentTime();   // 获取当前时间（月份含中文）
    QString getTimeAfterYears(int years,int months,int days);
    QString getFormattedTime(QDateTime dateTime);

private:
    QString formatDateTime(QDateTime dateTime); // 转为STK标准时间格式
    QString monthToEnglish(int month);
};

#endif // TIMEMANAGER_H
