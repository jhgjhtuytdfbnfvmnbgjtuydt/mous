#include <unistd.h>
#include <stdio.h>

#include <fstream>

#include <scx/Conv.h>
#include <scx/FileInfo.h>
using namespace scx;

#include <core/Conversion.h>
using namespace mous;

#include "cmd.h"
#include "ctx.h"

int cmd_dec(int argc, char** argv)
{
    // find wav encoder
    std::string wav_enc;
    const auto& encoders = ctx->converter.EncoderNames();
    for (const auto& encoder: encoders) {
        if (ToLower(encoder).find("wav") != std::string::npos) {
            wav_enc = encoder;
            break;
        }
    }
    if (wav_enc.empty()) {
        printf("can't find wav encoder!\n");
        return -1;
    }

    for (int i = 1; i < argc; ++i) {
        FileInfo info(argv[i]);
        if (!info.Exists() || info.Type() == FileType::Directory)
            continue;

        // load media file
        std::deque<MediaItem> media_list;
        ctx->mediaLoader.LoadMedia(argv[i], media_list);

        // convert each track
        for (size_t mi = 0; mi < media_list.size(); ++mi) {
            // output file name
            const char* outname = 
                (media_list.size() == 1 || media_list[mi].tag.title.empty()) ?
                (info.BaseName() + "." + std::to_string(mi) + ".wav").c_str() :
                (media_list[mi].tag.title + ".wav").c_str(); 
                 
            printf("save to: %s\n", outname);
            if (FileInfo(outname).Exists()) {
                printf("file already exist! overwrite? [n/y]\n");
                char ch;
                scanf("%c", &ch);
                if (ch != 'y') {
                    continue;
                }
            }

            // do it!
            auto conversion = ctx->converter.CreateConversion(media_list[mi], wav_enc);
            conversion->Run(outname);

            while (!conversion->IsFinished()) {
                double percent = conversion->Progress();
                if (percent < 0) {
                    printf("failed!\n");
                    break;
                }
                printf("\rprogress: %02d%% ", (int)(percent*100));
                usleep(200);
            }
            printf("\ndone!\n");
        }

        printf("\n");
    }

    return 0;
}

