#ifndef DDS_STREAM_TRANSFORMER
#define DDS_STREAM_TRANSFORMER

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavutil/imgutils.h>
  #include <libavutil/opt.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

#include <iostream>
#include <vector>
#include <fstream>

namespace PaaS {
class FFmpegDecoder {
private:
  AVCodecContext *codec_ctx;
  AVFrame *decoded_frame;
  AVPacket pkt;

public:
  FFmpegDecoder() {
    avcodec_register_all();
    codec_ctx = nullptr;
    decoded_frame = av_frame_alloc();
  }

  ~FFmpegDecoder() {
    av_frame_free(&decoded_frame);
    if (codec_ctx) {
      avcodec_free_context(&codec_ctx);
    }
  }

  bool initializeDecoder() {
    AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
      std::cerr << "Codec not found\n";
      return false;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
      std::cerr << "Could not allocate video codec context\n";
      return false;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
      std::cerr << "Could not open codec\n";
      return false;
    }

    return true;
  }

  bool decodeFrame(const std::vector<uint8_t> &input_data, int numerator,
                   int denominator, AVPacket &jpeg_pkt) {
    av_init_packet(&pkt);
    pkt.data = const_cast<uint8_t *>(input_data.data());
    pkt.size = input_data.size();

    int ret = 0;
    char err_buf[AV_ERROR_MAX_STRING_SIZE];

    if (avcodec_send_packet(codec_ctx, &pkt) < 0) {
      std::cerr << "Error sending a packet for decoding\n";
      return false;
    }

    if ((ret = avcodec_receive_frame(codec_ctx, decoded_frame)) < 0) {
      av_strerror(ret, err_buf, sizeof(err_buf));
      std::cerr << "Error during decoding: " << err_buf << std::endl;
      return false;
    }

    AVCodec *jpeg_codec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (!jpeg_codec) {
      std::cerr << "JPEG codec not found\n";
      return false;
    }

    AVCodecContext *jpeg_ctx = avcodec_alloc_context3(jpeg_codec);
    if (!jpeg_ctx) {
      std::cerr << "Could not allocate JPEG codec context\n";
      return false;
    }

    jpeg_ctx->pix_fmt = AV_PIX_FMT_YUVJ420P;
    jpeg_ctx->height = codec_ctx->height;
    jpeg_ctx->width = codec_ctx->width;
    jpeg_ctx->time_base = (AVRational){numerator, denominator};

    if (avcodec_open2(jpeg_ctx, jpeg_codec, nullptr) < 0) {
      std::cerr << "Could not open JPEG codec\n";
      avcodec_free_context(&jpeg_ctx);
      return false;
    }

    err_buf[AV_ERROR_MAX_STRING_SIZE];
    // 将帧发送到编码器
    ret = avcodec_send_frame(jpeg_ctx, decoded_frame);
    if (ret < 0) {
      av_strerror(ret, err_buf, sizeof(err_buf));
      std::cerr << "Error sending frame: " << err_buf << std::endl;
      // 处理错误并返回
    }

    ret = avcodec_receive_packet(jpeg_ctx, &jpeg_pkt);
    if (ret < 0) {
      av_strerror(ret, err_buf, sizeof(err_buf));
      std::cerr << "Error encoding to JPEG: " << err_buf << std::endl;
      avcodec_free_context(&jpeg_ctx);
      return false;
    }

    avcodec_free_context(&jpeg_ctx);
    return true;
  }
  /**
   * 由指標與長度建立std::vector
   * @name getVector
   * @param u_int8_t *ptr
   * @param size_t len
   * @return std::vector<u_int8_t>
  */
  std::vector<u_int8_t> getVector(u_int8_t *ptr, size_t len) {
    // Create a vector from an array
    std::vector<u_int8_t> vecObj(ptr, ptr + len);
    return vecObj;
  }
};
class H264Converter {
private:
    AVCodecContext *decode_ctx;
    AVCodecContext *encode_ctx;
    AVFrame *decoded_frame;
    AVFrame *scaled_frame;
    AVPacket pkt;
    SwsContext *sws_ctx;

public:
    H264Converter() {
        decode_ctx = nullptr;
        encode_ctx = nullptr;
        decoded_frame = av_frame_alloc();
        scaled_frame = av_frame_alloc();
        if (!decoded_frame || !scaled_frame) {
            std::cerr << "Could not allocate frame\n";
            return;
        }

        if (!initializeDecoder() || !initializeEncoder() || !initializeScaler()) {
            std::cerr << "Error initializing H264 converter\n";
            return;
        }
    }

    ~H264Converter() {
        av_frame_free(&decoded_frame);
        av_frame_free(&scaled_frame);
        if (decode_ctx) {
            avcodec_free_context(&decode_ctx);
        }
        if (encode_ctx) {
            avcodec_free_context(&encode_ctx);
        }
        if (sws_ctx) {
            sws_freeContext(sws_ctx);
        }
    }

    bool initializeDecoder() {
        AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "Codec not found\n";
            return false;
        }

        decode_ctx = avcodec_alloc_context3(codec);
        if (!decode_ctx) {
            std::cerr << "Could not allocate video codec context\n";
            return false;
        }
        decode_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        decode_ctx->height = 1080;
        decode_ctx->width = 1920;
        decode_ctx->time_base = (AVRational){1, 30};

        if (avcodec_open2(decode_ctx, codec, nullptr) < 0) {
            std::cerr << "Could not open codec\n";
            return false;
        }

        return true;
    }

    bool initializeEncoder() {
        AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
        if (!codec) {
            std::cerr << "Codec not found\n";
            return false;
        }

        encode_ctx = avcodec_alloc_context3(codec);
        if (!encode_ctx) {
            std::cerr << "Could not allocate video codec context\n";
            return false;
        }

        encode_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
        encode_ctx->height = 480;
        encode_ctx->width = 854;
        encode_ctx->time_base = (AVRational){1, 30};
        encode_ctx->bit_rate = 2000000;  // Adjust as needed

        av_opt_set(encode_ctx->priv_data, "preset", "ultrafast", 0);

        if (avcodec_open2(encode_ctx, codec, nullptr) < 0) {
            std::cerr << "Could not open codec\n";
            return false;
        }

        return true;
    }

    bool initializeScaler() {
        std::cout << "Decode context - Width: " << decode_ctx->width << ", Height: " << decode_ctx->height << ", Pixel Format: " << decode_ctx->pix_fmt << std::endl;
        std::cout << "Encode context - Width: " << encode_ctx->width << ", Height: " << encode_ctx->height << ", Pixel Format: " << encode_ctx->pix_fmt << std::endl;
        sws_ctx = sws_getContext(
            decode_ctx->width, decode_ctx->height, decode_ctx->pix_fmt,
            encode_ctx->width, encode_ctx->height, encode_ctx->pix_fmt,
            SWS_BICUBIC, nullptr, nullptr, nullptr
        );
        if (!sws_ctx) {
            std::cerr << "Could not initialize the conversion context\n";
            return false;
        }

        return true;
    }

    bool convertH264(const std::vector<uint8_t>& input_data, std::vector<uint8_t>& output_data) {
        int ret = 0;
        char err_buf[AV_ERROR_MAX_STRING_SIZE];
        static int64_t next_pts = 0;

        av_init_packet(&pkt);
        pkt.data = const_cast<uint8_t*>(input_data.data());
        pkt.size = input_data.size();

        if (avcodec_send_packet(decode_ctx, &pkt) < 0) {
            std::cerr << "Error sending a packet for decoding\n";
            return false;
        }

        if (avcodec_receive_frame(decode_ctx, decoded_frame) < 0) {
            std::cerr << "Error during decoding\n";
            return false;
        }

        // Allocate buffer for scaled frame
        int num_bytes = av_image_get_buffer_size(encode_ctx->pix_fmt, encode_ctx->width, encode_ctx->height, 1);
        uint8_t *buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
        av_image_fill_arrays(scaled_frame->data, scaled_frame->linesize, buffer, encode_ctx->pix_fmt, encode_ctx->width, encode_ctx->height, 1);

        // Scale the decoded frame to the output size
        sws_scale(sws_ctx, (const uint8_t * const*)decoded_frame->data, decoded_frame->linesize, 0, decode_ctx->height, scaled_frame->data, scaled_frame->linesize);

        scaled_frame->pts = next_pts++;
        ret = avcodec_send_frame(encode_ctx, scaled_frame);
        if (ret < 0) {
            av_strerror(ret, err_buf, sizeof(err_buf));
            std::cerr << "Error sending a frame for 480P encoding: " << err_buf << std::endl;
            std::cerr << "ret =  " << ret << " " << AVERROR_EOF << std::endl;
            if (ret != AVERROR_EOF) {
            std::cerr << "2ret =  " << ret << " " << AVERROR_EOF << std::endl;
                av_free(buffer);
                return false;
            }
        }
        // ret = avcodec_send_frame(encode_ctx, nullptr);  // Tell the encoder that there are no more frames
        // if (ret < 0) {
        //     std::cerr << "Error sending null frame for 480P encoding\n";
        // }

        AVPacket enc_pkt;
        av_init_packet(&enc_pkt);
        enc_pkt.data = nullptr;
        enc_pkt.size = 0;

        ret = avcodec_receive_packet(encode_ctx, &enc_pkt);
        if (ret < 0) {
            av_strerror(ret, err_buf, sizeof(err_buf));
            std::cerr << "Error during 480P encoding: " << err_buf << std::endl;
            std::cerr << "ret =  " << ret << " " << AVERROR_EOF << std::endl;
            if (ret != AVERROR_EOF) {
                av_free(buffer);
                return false;
            }
        }

        output_data.assign(enc_pkt.data, enc_pkt.data + enc_pkt.size);
        av_packet_unref(&enc_pkt);
        av_free(buffer);
        return true;
    }
};
} // namespace PaaS
#endif