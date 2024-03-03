#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "alsa/global.h"
#include "alsa/input.h"
#include "alsa/output.h" 
#include "alsa/conf.h"
#include "alsa/pcm.h"
#include "alsa/control.h"
#include "alsa/error.h"

const char* get_sndcard() 
{
    int card = -1;
    int err;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *info;
    static char hwdevice[15];

    while (snd_card_next(&card) == 0 && card >= 0) 
    {
        char cardname[32];
        sprintf(cardname, "hw:%d", card);

        if ((err = snd_ctl_open(&handle, cardname, 0)) < 0) 
        {
            fprintf(stderr, "Cannot open control for card %d: %s\n", card, snd_strerror(err));
            continue;
        }

        snd_ctl_card_info_alloca(&info);
        if ((err = snd_ctl_card_info(handle, info)) < 0) 
        {
            fprintf(stderr, "Cannot get card info for card %d: %s\n", card, snd_strerror(err));
            snd_ctl_close(handle);
            continue;
        }

        // List PCM devices
        int dev = -1;
        snd_pcm_info_t *pcm_info;
        snd_pcm_info_alloca(&pcm_info);
        while (snd_ctl_pcm_next_device(handle, &dev) == 0 && dev >= 0) 
        {
            snd_pcm_info_set_device(pcm_info, dev);
            snd_pcm_info_set_subdevice(pcm_info, 0);
            snd_pcm_info_set_stream(pcm_info, SND_PCM_STREAM_PLAYBACK);
            if (snd_ctl_pcm_info(handle, pcm_info) >= 0) 
            {
                if(strstr(snd_pcm_info_get_name(pcm_info),"HDMI") == NULL)
                {
                    snd_ctl_close(handle);
                    sprintf(hwdevice, "plughw:%d,0", card);
                    return hwdevice;
                }
            }
        }
        snd_ctl_close(handle);
    }
    return "plughw:0,0";
}
