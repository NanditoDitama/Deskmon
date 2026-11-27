#ifndef THEMEDICONPROVIDER_H
#define THEMEDICONPROVIDER_H

#include <QQuickImageProvider>
#include <QGuiApplication>
#include <QSvgRenderer>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QFile>
#include <QColor>
#include <QSize>
#include <QtMath>

class ThemedIconProvider : public QQuickImageProvider
{
public:
    ThemedIconProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        // format: "icon_name.svg?#00e0a8" or "icon_name.svg?color=#00e0a8" or "icon_name.svg"
        QString iconName = id;
        QString colorStr;

        int qIdx = id.indexOf('?');
        if (qIdx != -1) {
            iconName = id.left(qIdx);
            colorStr = id.mid(qIdx + 1);
            if (colorStr.startsWith("color=")) {
                colorStr = colorStr.mid(6);
            }
        }

        QString resPath = QString(":/icons/%1").arg(iconName);
        if (!QFile::exists(resPath)) {
            resPath = QString("resources/icons/%1").arg(iconName);
        }

        QSvgRenderer renderer(resPath);
        if (!renderer.isValid()) {
            return QPixmap();
        }

        const qreal dpr = qGuiApp ? qMax(qreal(2.0), qGuiApp->devicePixelRatio()) : qreal(2.0);
        int baseW = (requestedSize.width() > 0) ? requestedSize.width() : 64;
        int baseH = (requestedSize.height() > 0) ? requestedSize.height() : 64;
        int w = qBound(48, qRound(baseW * dpr), 256);
        int h = qBound(48, qRound(baseH * dpr), 256);

        QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        renderer.render(&painter);

        if (!colorStr.isEmpty()) {
            QColor color(colorStr);
            if (color.isValid()) {
                painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
                painter.fillRect(image.rect(), color);
            }
        }
        painter.end();

        if (size) {
            *size = QSize(w, h);
        }

        return QPixmap::fromImage(image);
    }
};

#endif // THEMEDICONPROVIDER_H
