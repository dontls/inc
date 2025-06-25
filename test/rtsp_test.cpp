#include "../rtsp.hpp"
#include "../mp4.hpp"
#include "../g711.h"
#include "../faac.hpp"
#include "../minimp4.hpp"

int main1(int argc, char const *argv[]) {
  libfile::Mp4 file("00rtsp.mp4");
  try {
    uint16_t pcm[1024] = {};
    libfaac::Encoder faac(8000, 1, 16, libfaac::STREAM_RAW);
    librtsp::Client cli(true);
    auto firts = libtime::UnixMilli();
    cli.Play("rtsp://admin:12345@172.16.50.219:554/test.mp4",
             [&](const char *format, uint8_t ftype, char *data, size_t len) {
               auto ts = libtime::UnixMilli();
               printf("%s %lld type %d size %lu\n", format, ts, ftype, len);
               if (ftype == 3) {
                 for (size_t i = 0; i < len; i++) {
                   pcm[i] = ulaw_to_linear(data[i]);
                 }
                 len *= 2;
                 data = faac.Encode((char *)pcm, len, len);
               }
               if (len > 0) {
                 file.WriteFrame(ts, ftype, data, len);
               }
               if (ts - firts > 5000) {
                 cli.Stop();
               }
             });
  } catch (libnet::Exception) {
  }
  file.Close();
  return 0;
}

static int write_callback(int64_t offset, const void *buffer, size_t size,
                          void *token) {
  FILE *f = (FILE *)token;
  fseek(f, offset, SEEK_SET);
  return fwrite(buffer, 1, size, f) != size;
}

int main2(int argc, char const *argv[]) {
  libfile::Mp4 file("00rtsp.mp4");
  mp4_h26x_writer_t mp4wr;
  FILE *fout = fopen("123344.mp4", "wb");
  MP4E_mux_t *mux = MP4E_open(0, 0, fout, write_callback);
  if (MP4E_STATUS_OK != mp4_h26x_write_init(&mp4wr, mux, 1280, 720, false)) {
    printf("error: mp4_h26x_write_init failed\n");
    exit(1);
  }
  MP4E_track_t tr;
  tr.track_media_kind = e_audio;
  tr.language[0] = 'u';
  tr.language[1] = 'n';
  tr.language[2] = 'd';
  tr.language[3] = 0;
  tr.object_type_indication = MP4_OBJECT_TYPE_AUDIO_ISO_IEC_14496_3;
  tr.time_scale = 90000;
  tr.default_duration = 0;
  tr.u.a.channelcount = 1;
  int audio_track_id = MP4E_add_track(mux, &tr);
  unsigned char dsi[2] = {0x15, 0x90};
  MP4E_set_dsi(mux, audio_track_id, dsi, 2);
  try {
    uint16_t pcm[1024] = {};
    libfaac::Encoder faac(8000, 1, 16, libfaac::STREAM_RAW);
    librtsp::Client cli(true);
    long long lsts = 0, firts = 0;
    cli.Play("rtsp://admin:12345@172.16.50.219:554/test.mp4",
             [&](const char *format, uint8_t ftype, char *data, size_t len) {
               auto ts = libtime::UnixMilli();
               if (lsts == 0) {
                 firts = lsts = ts;
               }
               if (ts - firts > 5000) {
                 cli.Stop();
               }
               printf("%s %lld type %d size %lu\n", format, ts, ftype, len);
               if (ftype == 3) {
                 for (size_t i = 0; i < len; i++) {
                   pcm[i] = ulaw_to_linear(data[i]);
                 }
                 len *= 2;
                 data = faac.Encode((char *)pcm, len, len);
                 if (len > 0) {
                   MP4E_put_sample(mux, audio_track_id, data, len,
                                   (ts - lsts) * 90, MP4E_SAMPLE_RANDOM_ACCESS);
                   file.WriteFrame(ts, ftype, data, len);
                 }
               } else {
                 file.WriteFrame(ts, ftype, data, len);
                 mp4_h26x_write_nal(&mp4wr, (unsigned char *)data, len,
                                    (ts - lsts) * 90);
               }
               lsts = ts;
             });
  } catch (libnet::Exception) {
  }
  MP4E_close(mux);
  mp4_h26x_write_close(&mp4wr);
  fclose(fout);
  file.Close();
  return 0;
}

int main(int argc, char const *argv[]) {
  // main1(argc, argv);
  main2(argc, argv);
}