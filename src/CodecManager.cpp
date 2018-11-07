#include "CodecManager.moc"
#include "common.h"

extern "C"
{
#include <libavutil/common.h>
#include <libavutil/pixdesc.h>


#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)
static enum AVPixelFormat get_hw_format_videotoolbox(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_VIDEOTOOLBOX)
            return *p;
    }
    fprintf(stderr, "Failed to get AV_PIX_FMT_VIDEOTOOLBOX surface format.\n");
    return AV_PIX_FMT_NONE;
}

static enum AVPixelFormat get_hw_format_directxva2(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_DXVA2_VLD)
            return *p;
    }
    fprintf(stderr, "Failed to get AV_PIX_FMT_DXVA2_VLD surface format.\n");
    return AV_PIX_FMT_NONE;
}

static enum AVPixelFormat get_hw_format_vaapi(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_VAAPI_VLD)
            return *p;
    }
    fprintf(stderr, "Failed to get AV_PIX_FMT_VAAPI_VLD surface format.\n");
    return AV_PIX_FMT_NONE;
}

static enum AVPixelFormat get_hw_format_vdpau(AVCodecContext *ctx,
                                        const enum AVPixelFormat *pix_fmts)
{
    const enum AVPixelFormat *p;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == AV_PIX_FMT_VDPAU)
            return *p;
    }
    fprintf(stderr, "Failed to get AV_PIX_FMT_VDPAU surface format.\n");
    return AV_PIX_FMT_NONE;
}
#endif

}

CodecManager *CodecManager::_instance = 0;

CodecManager::CodecManager(QObject *parent) : QObject(parent)
{

    av_register_all();
    avcodec_register_all();
    avfilter_register_all();
    avdevice_register_all();

#ifndef NDEBUG
    /* print warning info from ffmpeg */
    av_log_set_level( AV_LOG_WARNING  );
#else
    av_log_set_level( AV_LOG_QUIET ); /* don't print warnings from ffmpeg */
#endif

    // disabled by default
    _useHardwareAcceleration = false;
}


void CodecManager::registerAll()
{
    if (_instance == 0) {
        _instance = new CodecManager();
    }
}

void CodecManager::printError(QString streamname, QString message, int err)
{
    QString errormessage;
    switch (err)
    {
    case AVERROR_INVALIDDATA:
        errormessage = tr("Invalid data while parsing header.");
        break;
    case AVERROR(EIO):
        errormessage =  tr("I/O error. The file is corrupted or the stream is unavailable.");
        break;
    case AVERROR(ENOMEM):
        errormessage =  tr("Memory allocation error.");
        break;
    case AVERROR(ENOENT):
        errormessage =  tr("No such entry.");
        break;
    default:
        errormessage =  tr("Stream or format unavailable.");
        break;
    }

    qWarning() << streamname << QChar(124).toLatin1()<< message << errormessage;
}

bool CodecManager::openFormatContext(AVFormatContext **_pFormatCtx, QString streamToOpen, QString streamFormat, QHash<QString, QString> streamOptions)
{
    registerAll();

    int err = 0;

    // Check context
    if ( !_pFormatCtx || !*_pFormatCtx)
        return false;

    AVInputFormat *ifmt = NULL;
    if (!streamFormat.isEmpty())
        ifmt = av_find_input_format(qPrintable(streamFormat));

    AVDictionary* options = NULL;
    QHashIterator<QString, QString> i(streamOptions);
    while (i.hasNext()) {
        i.next();
        av_dict_set(&options,qPrintable(i.key()), qPrintable(i.value()),0);
    }

    // open stream
    err = avformat_open_input(_pFormatCtx, qPrintable(streamToOpen), ifmt, &options);
    if (err < 0) {
        printError(streamFormat+" "+streamToOpen, "Error opening :", err);
        return false;
    }

    // fill info stream
    err = avformat_find_stream_info(*_pFormatCtx, NULL);
    if (err < 0) {
        printError(streamFormat+" "+streamToOpen, "Error setting up :", err);
        return false;
    }

    return true;
}

double CodecManager::getDurationStream(AVFormatContext *codeccontext, int stream)
{
    double d = 0.0;

    // get duration from stream
    if (codeccontext->streams[stream] && codeccontext->streams[stream]->duration != (int64_t) AV_NOPTS_VALUE ) {
        d = double(codeccontext->streams[stream]->duration) * av_q2d(codeccontext->streams[stream]->time_base);
    }
    // else try to get the duration from context (codec info)
    else if (codeccontext && codeccontext->duration != (int64_t) AV_NOPTS_VALUE ) {
        d = double(codeccontext->duration) * av_q2d(AV_TIME_BASE_Q);
    }
    // else try to get duration from frame rate
    else {
        double fps = CodecManager::getFrameRateStream(codeccontext, stream);
        if ( fps > 0 )
            d = 1.0 / fps;
    }

    // inform in case duration of file is certainly a bad estimate
    if (codeccontext->duration_estimation_method == AVFMT_DURATION_FROM_BITRATE && codeccontext->streams[stream]->nb_frames > 2) {
        qWarning() << codeccontext->url << QChar(124).toLatin1()<< tr("Unspecified duration in codec.");
    }

    return d;
}


double CodecManager::getFrameRateStream(AVFormatContext *codeccontext, int stream)
{
    double d = 1.0;

    // get guessed framerate from libav, if correct. (deprecated)
#if FF_API_R_FRAME_RATE
    if (codeccontext->streams[stream] )
        d = av_q2d( av_stream_get_r_frame_rate(codeccontext->streams[stream]) );
#endif
    // get average fps from stream
    else if (codeccontext->streams[stream] && codeccontext->streams[stream]->avg_frame_rate.den > 0)
        d = av_q2d(codeccontext->streams[stream]->avg_frame_rate);

    return d;
}

int CodecManager::countFrames(AVFormatContext *codeccontext, int stream, AVCodecContext *decoder)
{
    int n = 0;
    bool eof = false;
    AVPacket pkt1, *pkt = &pkt1;
    AVFrame *tmpframe = av_frame_alloc();

    while (!eof && n < 500 )
    {
        av_frame_unref(tmpframe);
        int ret = av_read_frame(codeccontext, pkt);
        if (ret < 0) {
            if (ret == AVERROR_EOF)
                eof = true;
            else
                continue;
        }

        if (pkt->stream_index != stream)
            continue;

        if ( avcodec_send_packet(decoder, pkt) < 0 )
            continue;

        av_packet_unref(pkt);

        // read until the frame is finished
        int frameFinished = AVERROR(EAGAIN);
        while ( frameFinished < 0 )
        {
            frameFinished = avcodec_receive_frame(decoder, tmpframe);

            if ( frameFinished == AVERROR(EAGAIN) )
                break;
            else if ( frameFinished == AVERROR_EOF ||  frameFinished < 0 ) {
                eof = true;
                break;
            }

            ++n;
        }
    }

    // free frame
    av_frame_unref(tmpframe);
    // rewind
    av_seek_frame(codeccontext, stream, 0, AVSEEK_FLAG_BACKWARD);
    // flush decoder
    avcodec_flush_buffers(decoder);

    return n;
}


void CodecManager::convertSizePowerOfTwo(int &width, int &height)
{
    width = roundPowerOfTwo(width);
    height = roundPowerOfTwo(height);
}


void CodecManager::displayFormatsCodecsInformation(QString iconfile)
{
    registerAll();

    QVBoxLayout *verticalLayout;
    QLabel *label;
    QPlainTextEdit *options;
    QLabel *label_2;
    QTreeWidget *availableFormatsTreeWidget;
    QLabel *label_3;
    QTreeWidget *availableCodecsTreeWidget;
    QDialogButtonBox *buttonBox;

    QDialog *ffmpegInfoDialog = new QDialog;
    QIcon icon;
    icon.addFile(iconfile, QSize(), QIcon::Normal, QIcon::Off);
    ffmpegInfoDialog->setWindowIcon(icon);
    ffmpegInfoDialog->resize(450, 600);
    verticalLayout = new QVBoxLayout(ffmpegInfoDialog);
    label = new QLabel(ffmpegInfoDialog);
    label_2 = new QLabel(ffmpegInfoDialog);
    label_3 = new QLabel(ffmpegInfoDialog);

    availableFormatsTreeWidget = new QTreeWidget(ffmpegInfoDialog);
    availableFormatsTreeWidget->setAlternatingRowColors(true);
    availableFormatsTreeWidget->setRootIsDecorated(false);
    availableFormatsTreeWidget->header()->setVisible(true);

    availableCodecsTreeWidget = new QTreeWidget(ffmpegInfoDialog);
    availableCodecsTreeWidget->setAlternatingRowColors(true);
    availableCodecsTreeWidget->setRootIsDecorated(false);
    availableCodecsTreeWidget->header()->setVisible(true);

    buttonBox = new QDialogButtonBox(ffmpegInfoDialog);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Close);
    QObject::connect(buttonBox, SIGNAL(rejected()), ffmpegInfoDialog, SLOT(reject()));

    ffmpegInfoDialog->setWindowTitle(tr("About Libav formats and codecs"));

    label->setWordWrap(true);
    label->setOpenExternalLinks(true);
    label->setTextFormat(Qt::RichText);
    label->setText(tr( "<H3>About Libav</H3>"
                       "<br>This program uses <b>libavcodec %1.%2.%3</b>."
                       "<br><br><b>Libav</b> provides cross-platform tools and libraries to convert, manipulate and "
                       "stream a wide range of multimedia formats and protocols."
                       "<br>For more information : <a href=\"http://www.libav.org\">http://www.libav.org</a>"
                       "<br><br>Compilation options:").arg(LIBAVCODEC_VERSION_MAJOR).arg(LIBAVCODEC_VERSION_MINOR).arg( LIBAVCODEC_VERSION_MICRO));

    options = new QPlainTextEdit( QString(avcodec_configuration()), ffmpegInfoDialog);

    label_2->setText( tr("Available codecs:"));
    label_3->setText( tr("Available formats:"));

    QTreeWidgetItem *title = availableFormatsTreeWidget->headerItem();
    title->setText(1, tr("Description"));
    title->setText(0, tr("Name"));

    title = availableCodecsTreeWidget->headerItem();
    title->setText(1, tr("Description"));
    title->setText(0, tr("Name"));

    QTreeWidgetItem *formatitem;
    AVInputFormat *ifmt = NULL;
    AVCodec *p = NULL, *p2;
    const char *last_name;

    last_name = "000";
    for (;;)
    {
        const char *name = NULL;
        const char *long_name = NULL;

        while ((ifmt = av_iformat_next(ifmt)))
        {
            if ((name == NULL || strcmp(ifmt->name, name) < 0) && strcmp(
                        ifmt->name, last_name) > 0)
            {
                name = ifmt->name;
                long_name = ifmt->long_name;
            }
        }
        if (name == NULL)
            break;
        last_name = name;

        formatitem = new QTreeWidgetItem(availableFormatsTreeWidget);
        formatitem->setText(0, QString(name));
        formatitem->setText(1, QString(long_name));
        availableFormatsTreeWidget->addTopLevelItem(formatitem);
    }

    last_name = "000";
    for (;;)
    {
        int decode = 0;
        int cap = 0;

        p2 = NULL;
        while ((p = av_codec_next(p)))
        {
            if ((p2 == NULL || strcmp(p->name, p2->name) < 0) && strcmp(
                        p->name, last_name) > 0)
            {
                p2 = p;
                decode = cap = 0;
            }
            if (p2 && strcmp(p->name, p2->name) == 0)
            {
                if (p->decode)
                    decode = 1;
                cap |= p->capabilities;
            }
        }
        if (p2 == NULL)
            break;
        last_name = p2->name;

        if (decode && p2->type == AVMEDIA_TYPE_VIDEO)
        {
            formatitem = new QTreeWidgetItem(availableCodecsTreeWidget);
            formatitem->setText(0, QString(p2->name));
            formatitem->setText(1, QString(p2->long_name));
            availableCodecsTreeWidget->addTopLevelItem(formatitem);
        }
    }

    verticalLayout->setSpacing(10);
    verticalLayout->addWidget(label);
    verticalLayout->addWidget(options);
    verticalLayout->addWidget(label_2);
    verticalLayout->addWidget(availableCodecsTreeWidget);
    verticalLayout->addWidget(label_3);
    verticalLayout->addWidget(availableFormatsTreeWidget);
    verticalLayout->addWidget(buttonBox);

    ffmpegInfoDialog->exec();

    delete verticalLayout;
    delete label;
    delete label_2;
    delete label_3;
    delete options;
    delete availableFormatsTreeWidget;
    delete availableCodecsTreeWidget;
    delete buttonBox;
    delete ffmpegInfoDialog;
}

QString CodecManager::getPixelFormatName(AVPixelFormat pix_fmt)
{
    QString pfn = "Unknown";
    registerAll();

    pfn = QString(av_pix_fmt_desc_get(pix_fmt)->name);
    pfn += QString(" (%1 bpp)").arg(av_get_bits_per_pixel( av_pix_fmt_desc_get(pix_fmt)) );

    return pfn;
}

bool CodecManager::pixelFormatHasAlphaChannel(AVPixelFormat pix_fmt)
{
    registerAll();

    if (pix_fmt < 0)
        return false;

    return  (  (av_pix_fmt_desc_get(pix_fmt)->nb_components > 3)
            // does the format has ALPHA ?
            || ( av_pix_fmt_desc_get(pix_fmt)->flags & AV_PIX_FMT_FLAG_ALPHA )
            // special case of PALLETE and GREY pixel formats(converters exist for rgba)
           // || ( av_pix_fmt_desc_get(pix_fmt)->flags & AV_PIX_FMT_FLAG_PAL )
               );

}

bool CodecManager::useHardwareAcceleration()
{
    registerAll();

    return _instance->_useHardwareAcceleration;
}

void CodecManager::setHardwareAcceleration(bool use)
{
    registerAll();

    _instance->_useHardwareAcceleration = use;
}

// see https://trac.ffmpeg.org/wiki/HWAccelIntro

const AVCodecHWConfig *CodecManager::getCodecHardwareAcceleration(AVCodec *codec)
{
    if (!useHardwareAcceleration())
        return 0;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)

#ifdef Q_OS_WIN
    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_DXVA2;
#else
#ifdef Q_OS_MAC
    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_VIDEOTOOLBOX;
#else
    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_VDPAU;
//    enum AVHWDeviceType type = AV_HWDEVICE_TYPE_VAAPI;
#endif
#endif

    // look for hardware config matching the appropriate type
    for (int i = 0; i < 2 ; ++i) {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (config && config->device_type == type) {
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) {
                return config;
            }
        }
    }
#endif

    return 0;
}


void CodecManager::applyCodecHardwareAcceleration(AVCodecContext *CodecContext, const AVCodecHWConfig *config)
{
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(58,0,0)

    if (!config || !CodecContext)
        return;

    // deal with harware pixel format
    enum AVPixelFormat hw_pix_fmt;
    hw_pix_fmt = config->pix_fmt;

    // Create a hw context
    AVBufferRef *hw_device_ctx = NULL;
    if ( av_hwdevice_ctx_create(&hw_device_ctx, config->device_type, NULL, NULL, 0) == 0)
    {
        // set function for pixel format
        switch (config->pix_fmt) {
        case AV_PIX_FMT_VIDEOTOOLBOX:
            CodecContext->get_format  = get_hw_format_videotoolbox;
            break;
        case AV_PIX_FMT_DXVA2_VLD:
            CodecContext->get_format  = get_hw_format_directxva2;
            break;
        case AV_PIX_FMT_VAAPI_VLD:
            CodecContext->get_format  = get_hw_format_vaapi;
            break;
        case AV_PIX_FMT_VDPAU:
            CodecContext->get_format  = get_hw_format_vdpau;
            break;
        default:
            hw_pix_fmt = AV_PIX_FMT_NONE;
        }

        // set hw decoding context to the decoder
        if ( hw_pix_fmt != AV_PIX_FMT_NONE ) {

            // copy hardware device context
            CodecContext->hw_device_ctx = av_buffer_ref(hw_device_ctx);

            // free tmp buffer
            av_buffer_unref(&hw_device_ctx);
        }
    }

#endif

}

bool CodecManager::supportsHardwareAcceleratedCodec(QString filename)
{
    bool frameFilled = false;
    AVFormatContext *pFormatCtx;
    pFormatCtx = avformat_alloc_context();

    if ( CodecManager::openFormatContext( &pFormatCtx, filename) ) {

        int videoStream = 0;
        AVCodec *codec = NULL;
        videoStream = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
        if (videoStream >= 0 && codec != NULL) {

            const AVCodecHWConfig *hwconfig = getCodecHardwareAcceleration(codec);
            if (hwconfig != NULL) {

                AVCodecContext *hw_video_dec = avcodec_alloc_context3(codec);
                if (hw_video_dec) {

                    if (avcodec_parameters_to_context(hw_video_dec, pFormatCtx->streams[videoStream]->codecpar) >= 0) {

                        applyCodecHardwareAcceleration(hw_video_dec, hwconfig);
                        if ( hw_video_dec->hw_device_ctx != NULL ) {

                            if (avcodec_open2(hw_video_dec, codec, NULL) >= 0) {

                                // ok, let's try the hw codec
                                int trial = 0;
                                AVPacket pkt1, *pkt = &pkt1;
                                AVFrame *tmpframe = av_frame_alloc();
                                while (!frameFilled && ++trial < 50)
                                {
                                    if ( av_read_frame(pFormatCtx, pkt) < 0)
                                        break;
                                    if ( pkt->stream_index != videoStream )
                                        continue;
                                    if ( avcodec_send_packet(hw_video_dec, pkt) < 0 )
                                        break;

                                    int ret = avcodec_receive_frame(hw_video_dec, tmpframe);
                                    if (ret == AVERROR(EAGAIN) )
                                        continue;
                                    if (ret < 0 )
                                        break;

                                    // eventually managed to fill a frame !
                                    frameFilled = true;
                                }
                            }
                        }
                    }
                    avcodec_free_context(&hw_video_dec);
                }
            }
        }
        avformat_close_input(&pFormatCtx);
    }

    return frameFilled;
}



QHash<QString, QString> CodecManager::getDeviceList(QString formatname)
{
    registerAll();

    QHash<QString, QString> devices;

    AVFormatContext *dev = NULL;
    AVDeviceInfoList *device_list = NULL;

    AVInputFormat *fmt = av_find_input_format(qPrintable(formatname));
    if (!fmt)
    {
        qWarning() << "getRawDeviceListGeneric" << QChar(124).toLatin1()<< tr("Cannot find format %1.").arg(formatname);
        return devices;
    }

    if ( !fmt->priv_class || !AV_IS_INPUT_DEVICE(fmt->priv_class->category))
    {
        qWarning() << "getRawDeviceListGeneric" << QChar(124).toLatin1()<< tr("Not an input format %1.").arg(formatname);
        return devices;
    }

    if (!fmt->get_device_list) {
        qWarning() << "getRawDeviceListGeneric" << QChar(124).toLatin1()<< tr("Cannot list sources. Not implemented.");
       // return devices;
    }

    if (!(dev = avformat_alloc_context())){
        qWarning() << "getRawDeviceListGeneric" << QChar(124).toLatin1()<< tr("Cannot allocate device %1").arg(fmt->name);
        return devices;
    }
    dev->iformat = fmt;
    if (dev->iformat->priv_data_size > 0) {
        dev->priv_data = av_mallocz(dev->iformat->priv_data_size);
        if (!dev->priv_data) {
            avformat_free_context(dev);
            return devices;
        }
        if (dev->iformat->priv_class) {
            *(const AVClass**)dev->priv_data = dev->iformat->priv_class;
            av_opt_set_defaults(dev->priv_data);
        }
    } else {
        dev->priv_data = NULL;
    }

    AVDictionary* tmp = nullptr;
    av_dict_copy(&tmp, nullptr, 0);
    if (av_opt_set_dict2(dev, &tmp, AV_OPT_SEARCH_CHILDREN) < 0) {
        av_dict_free(&tmp);
        return devices;
    }

    if ( avdevice_list_devices(dev, &device_list) < 0 ) {
        qWarning() << "getRawDeviceListGeneric" << QChar(124).toLatin1()<< tr("Cannot list device %1").arg(fmt->name);
        return devices;
    }

    for (int i = 0; i < device_list->nb_devices; i++) {
        devices[ QString(device_list->devices[i]->device_name)] = QString(device_list->devices[i]->device_description);
    }

    avdevice_free_list_devices(&device_list);
    avformat_free_context(dev);

    return devices;
}

bool CodecManager::hasFormat(QString formatname)
{
    registerAll();

    AVInputFormat *fmt = av_find_input_format(qPrintable(formatname));

    return fmt != NULL;
}
