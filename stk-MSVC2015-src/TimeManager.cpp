#include "TimeManager.h"

TimeManager::TimeManager() {}
TimeManager::~TimeManager() {}

QString TimeManager::getCurrentTime() {
    QDateTime currentDateTime = QDateTime::currentDateTime();
    return formatDateTime(currentDateTime);
}

// 设置years年months月days后
QString TimeManager::getTimeAfterYears(int years,int months,int days) {
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QDateTime futureDateTime = currentDateTime.addYears(years).addMonths(months).addDays(days);
    return formatDateTime(futureDateTime);
}

QString TimeManager::getFormattedTime(QDateTime dateTime) {
    return formatDateTime(dateTime);
}

QString TimeManager::formatDateTime(QDateTime dateTime) {
    QString day = dateTime.date().toString("dd");
    QString month = monthToEnglish(dateTime.date().month());
    QString year = dateTime.date().toString("yyyy");
    QString time = "00:00:00.000";  // 配置具体几点钟开始
    return day + " " + month + " " + year + " " + time;
}

// 月份转英文
QString TimeManager::monthToEnglish(int month) {
    static const QStringList months = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return months[month - 1];
}
