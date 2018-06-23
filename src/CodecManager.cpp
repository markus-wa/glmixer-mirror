#include "CodecManager.moc"


extern "C"
{
#include <libavutil/common.h>
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(52,30,0)
#include <libavutil/pixdesc.h>
#endif
}

CodecManager *CodecManager::_instance = 0;


int roundPowerOfTwo(int v)
{
    int k;
    if (v == 0)
        return 1;
    for (k = sizeof(int) * 8 - 1; ((static_cast<int> (1U) << k) & v) == 0; k--)
        ;
    if (((static_cast<int> (1U) << (k - 1)) & v) == 0)
        return static_cast<int> (1U) << k;
    return static_cast<int> (1U) << (k + 1);
}

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
        errormessage =  tr("Unsupported format.");
        break;
    }

    qWarning() << streamname << QChar(124).toLatin1()<< message << errormessage;
}

bool CodecManager::openFormatContext(AVFormatContext **_pFormatCtx, QString streamToOpen, QString streamFormat)
{
    registerAll();

    int err = 0;

    // Check file
    if ( !_pFormatCtx || !*_pFormatCtx)
        return false;

    AVInputFormat *ifmt = NULL;
    if (!streamFormat.isEmpty())
        ifmt = av_find_input_format(qPrintable(streamFormat));

    // open stream
    err = avformat_open_input(_pFormatCtx, qPrintable(streamToOpen), ifmt, NULL);
    if (err < 0)
    {
        printError(streamToOpen, "Error opening :", err);
        return false;
    }

    // request to generate PTS
    (*_pFormatCtx)->flags |= AVFMT_FLAG_GENPTS;

    // fill info stream
    err = avformat_find_stream_info(*_pFormatCtx, NULL);
    if (err < 0)
    {
        printError(streamToOpen, "Error setting up :", err);
        return false;
    }

    return true;
}


// TODO : use av_find_best_stream instead of my own implementation ?
int CodecManager::getVideoStream(AVFormatContext *codeccontext)
{
    // Find the first video stream index
    int stream_index = -1;

    stream_index = av_find_best_stream(codeccontext, AVMEDIA_TYPE_VIDEO,  -1, -1, NULL, 0);

    return stream_index;
}

double CodecManager::getDurationStream(AVFormatContext *codeccontext, int stream)
{
    double d = 0.0;

    // get duration from stream
    if (codeccontext->streams[stream] && codeccontext->streams[stream]->duration != (int64_t) AV_NOPTS_VALUE )
        d = double(codeccontext->streams[stream]->duration) * av_q2d(codeccontext->streams[stream]->time_base);
    // else try to get the duration from context (codec info)
    else if (codeccontext && codeccontext->duration != (int64_t) AV_NOPTS_VALUE )
        d = double(codeccontext->duration) * av_q2d(AV_TIME_BASE_Q);
    // else try to get duration from frame rate
    else {
        double fps = CodecManager::getFrameRateStream(codeccontext, stream);
        if ( fps > 0 )
            d = 1.0 / fps;
    }

    // inform in case duration of file is certainly a bad estimate
    if (codeccontext->duration_estimation_method == AVFMT_DURATION_FROM_BITRATE && codeccontext->streams[stream]->nb_frames > 2) {
        qWarning() << codeccontext->filename << QChar(124).toLatin1()<< tr("Unspecified duration in codec.");
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
            || ( av_pix_fmt_desc_get(pix_fmt)->flags & AV_PIX_FMT_FLAG_PAL ) );

}


AVCodec *CodecManager::getEquivalentHardwareAcceleratedCodec(AVCodec *codec)
{
    AVCodec *hwcodec = NULL;

    // see http://net-zeal.de/hardware-acceleration-for-video-encoding-decoding-with-ffmpeg/
    char newcodecname[128];
#ifdef Q_OS_MAC
    // not applicable
    return NULL;
#else
//    return NULL;
    snprintf(newcodecname, 128, "%s_cuvid", codec->name);
#endif

    hwcodec = avcodec_find_decoder_by_name(newcodecname);

    return hwcodec;
}
