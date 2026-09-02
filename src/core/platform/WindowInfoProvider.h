#ifndef WINDOWINFOPROVIDER_H
#define WINDOWINFOPROVIDER_H

#include <QString>

struct WindowInfo {
    QString appName;
    QString title;
    QString url;
};

class WindowInfoProvider
{
public:
    static WindowInfo getActiveWindowInfo();
};

#endif // WINDOWINFOPROVIDER_H
