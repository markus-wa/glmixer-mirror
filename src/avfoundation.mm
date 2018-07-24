
#include "avfoundation.h"

#import <AVFoundation/AVFoundation.h>

QString NSStringToQString(const NSString *nsstr)
{
    NSRange range;
    range.location = 0;
    range.length = [nsstr length];
    //QString result(range.length, QChar(0));
    
    unichar *chars = new unichar[range.length];
    [nsstr getCharacters:chars range:range];
    QString result = QString::fromUtf16(chars, range.length);
    delete[] chars;
    return result;
}

QHash<QString, QString> avfoundation::getDeviceList()
{
    QHash<QString, QString> result;
    int d = 0;
    NSArray* devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice* device in devices) {
        result[ QString("%1:").arg(d++) ] = NSStringToQString([device localizedName]);
    }

    return result;
}

QHash<QString, QString> avfoundation::getScreenList()
{
    int d = 0;
    QHash<QString, QString> devices = avfoundation::getDeviceList();
    d = devices.size();

    QHash<QString, QString> result;
    uint32_t numScreens = 0;
    CGGetActiveDisplayList(0, NULL, &numScreens);
    if (numScreens > 0) {
        CGDirectDisplayID screens[numScreens];
        CGGetActiveDisplayList(numScreens, screens, &numScreens);
        for (uint32_t i = 0; i < numScreens; i++) {
            result[ QString("%1:").arg(d + i) ] = QString("Capture screen %1").arg(i);
        }
    }

    return result;
}
