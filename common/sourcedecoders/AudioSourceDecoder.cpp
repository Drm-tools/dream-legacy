/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Audio source encoder/decoder
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 1111
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include "AudioSourceDecoder.h"
#include <iostream>

// dummy AAC Decoder implementation if dll not found

// TODO - make this check audio frame CRCs
/*
aac_super_frame(audio_info, robustness_mode) //audio info from the SDC
{
if (robustness_mode == A ¦ B ¦ C ¦ D) {
switch (audio_info.audio_sampling_rate) { //only 12 000 and 24 000 is allowed
case 12 000: num_frames = 5;
break;
case 24 000: num_frames = 10;
break;
}
}
else { //robustness_mode == E
switch (audio_info.audio_sampling_rate) { //only 24 000 and 48 000 is allowed
case 24 000: num_frames = 5;
break;
case 48 000: num_frames = 10;
break;
}
}
aac_super_frame_header(num_frames - 1);
for (f = 0; f < num_frames; f++) {
// higher_protected_block
for (b = 0; b < num_higher_protected_bytes; b++)
audio_frame[f][b] 8
aac_crc_bits[f] 8 see annex D
}
//lower_protected_part
for (f = 0; f < num_frames; f++) {
num_lower_protected_bytes = frame_length[f] - num_higher_protected_bytes;
for (b = 0; b < num_lower_protected_bytes; b++)
audio_frame[f][num_higher_protected_bytes + b] 8
}
}

aac_super_frame_header(num_borders)
{
previous_border = 0;
for (n = 0; n < num_borders; n++) {
frame_length[n] = frame_border - previous_border; // frame border in bytes 12 2
previous_border = frame_border;
}
frame_length[num_borders] = audio_payload_length - previous_border;
if (num_borders == 9)
reserved // byte-alignment 4
}
*/

struct bitfile
{
    /* bit input */
    uint32_t bufa;
    uint32_t bufb;
    uint32_t bits_left;
    uint32_t buffer_size; /* size of the buffer in bytes */
    uint32_t bytes_left;
    uint8_t error;
    uint32_t *tail;
    uint32_t *start;
    const void *buffer;
};

#define MAX_CHANNELS        64
#define MAX_WINDOW_GROUPS    8
#define MAX_SFB             51

struct drc_info
{
    uint8_t present;

    uint8_t num_bands;
    uint8_t pce_instance_tag;
    uint8_t excluded_chns_present;
    uint8_t band_top[17];
    uint8_t prog_ref_level;
    uint8_t dyn_rng_sgn[17];
    uint8_t dyn_rng_ctl[17];
    uint8_t exclude_mask[MAX_CHANNELS];
    uint8_t additional_excluded_chns[MAX_CHANNELS];

    double ctrl1;
    double ctrl2;
};

struct program_config
{
    uint8_t element_instance_tag;
    uint8_t object_type;
    uint8_t sf_index;
    uint8_t num_front_channel_elements;
    uint8_t num_side_channel_elements;
    uint8_t num_back_channel_elements;
    uint8_t num_lfe_channel_elements;
    uint8_t num_assoc_data_elements;
    uint8_t num_valid_cc_elements;
    uint8_t mono_mixdown_present;
    uint8_t mono_mixdown_element_number;
    uint8_t stereo_mixdown_present;
    uint8_t stereo_mixdown_element_number;
    uint8_t matrix_mixdown_idx_present;
    uint8_t pseudo_surround_enable;
    uint8_t matrix_mixdown_idx;
    uint8_t front_element_is_cpe[16];
    uint8_t front_element_tag_select[16];
    uint8_t side_element_is_cpe[16];
    uint8_t side_element_tag_select[16];
    uint8_t back_element_is_cpe[16];
    uint8_t back_element_tag_select[16];
    uint8_t lfe_element_tag_select[16];
    uint8_t assoc_data_element_tag_select[16];
    uint8_t cc_element_is_ind_sw[16];
    uint8_t valid_cc_element_tag_select[16];

    uint8_t channels;

    uint8_t comment_field_bytes;
    uint8_t comment_field_data[257];

    /* extra added values */
    uint8_t num_front_channels;
    uint8_t num_side_channels;
    uint8_t num_back_channels;
    uint8_t num_lfe_channels;
    uint8_t sce_channel[16];
    uint8_t cpe_channel[16];
};

struct ic_stream
{
    uint8_t max_sfb;

    uint8_t num_swb;
    uint8_t num_window_groups;
    uint8_t num_windows;
    uint8_t window_sequence;
    uint8_t window_group_length[8];
    uint8_t window_shape;
    uint8_t scale_factor_grouping;
    uint16_t sect_sfb_offset[8][15*8];
    uint16_t swb_offset[52];
    uint16_t swb_offset_max;

    uint8_t sect_cb[8][15*8];
    uint16_t sect_start[8][15*8];
    uint16_t sect_end[8][15*8];
    uint8_t sfb_cb[8][8*15];
    uint8_t num_sec[8]; /* number of sections in a group */

    uint8_t global_gain;
    int16_t scale_factors[8][51]; /* [0..255] */

    uint8_t ms_mask_present;
    uint8_t ms_used[MAX_WINDOW_GROUPS][MAX_SFB];

    uint8_t noise_used;
    uint8_t is_used;

    uint8_t pulse_data_present;
    uint8_t tns_data_present;
    uint8_t gain_control_data_present;
    uint8_t predictor_data_present;

    pulse_info pul;
    tns_info tns;
#ifdef MAIN_DEC
    pred_info pred;
#endif
#ifdef LTP_DEC
    ltp_info ltp;
    ltp_info ltp2;
#endif
#ifdef SSR_DEC
    ssr_info ssr;
#endif

#ifdef ERROR_RESILIENCE
    /* ER HCR data */
    uint16_t length_of_reordered_spectral_data;
    uint8_t length_of_longest_codeword;
    /* ER RLVC data */
    uint8_t sf_concealment;
    uint8_t rev_global_gain;
    uint16_t length_of_rvlc_sf;
    uint16_t dpcm_noise_nrg;
    uint8_t sf_escapes_present;
    uint8_t length_of_rvlc_escapes;
    uint16_t dpcm_noise_last_position;
#endif
}; /* individual channel stream */

struct AACCRCDecoder
{

AACCRCDecoder()
{
}

~AACCRCDecoder()
{
}

char Init(unsigned long samplerate, unsigned char channels)
{
    /* Special object type defined for DRM */
    defObjectType = 27; // DRM_ER_LC;

	defSampleRate = samplerate;
    aacSectionDataResilienceFlag = 1; /* VCB11 */
    aacScalefactorDataResilienceFlag = 0; /* no RVLC */
    aacSpectralDataResilienceFlag = 1; /* HCR */
    frameLength = 960;
    // sf_index = get_sr_index(defSampleRate);
    object_type = defObjectType;

    if ((channels == DRMCH_STEREO) || (channels == DRMCH_SBR_STEREO))
        channelConfiguration = 2;
    else
        channelConfiguration = 1;

    if ((channels == DRMCH_MONO) || (channels == DRMCH_STEREO))
        sbr_present_flag = 0;
    else
        sbr_present_flag = 1;

    //fb = filter_bank_init(frameLength);

    return 0;
}


/* Table 4.4.13 ASME */
void DRM_aac_scalable_main_element(NeAACDecFrameInfo *hInfo,
                                   bitfile *ld, program_config *pce, drc_info *drc)
{
    uint8_t retval = 0;
    uint8_t channels = fr_channels = 0;
    uint8_t ch;
    uint8_t this_layer_stereo = (channelConfiguration > 1) ? 1 : 0;
    element cpe = {0};
    ic_stream *ics1 = &(cpe.ics1);
    ic_stream *ics2 = &(cpe.ics2);
    int16_t *spec_data;
    ALIGN int16_t spec_data1[1024] = {0};
    ALIGN int16_t spec_data2[1024] = {0};

    fr_ch_ele = 0;

    hInfo->error = DRM_aac_scalable_main_header(hDecoder, ics1, ics2, ld, this_layer_stereo);
    if (hInfo->error > 0)
        return;

    cpe.common_window = 1;
    if (this_layer_stereo)
    {
        element_id[0] = ID_CPE;
        if (element_output_channels[fr_ch_ele] == 0)
            element_output_channels[fr_ch_ele] = 2;
    } else {
        element_id[0] = ID_SCE;
    }

    if (this_layer_stereo)
    {
        cpe.channel        = 0;
        cpe.paired_channel = 1;
    }


    /* Stereo2 / Mono1 */
    ics1->tns_data_present = faad_get1bit(ld);

#if defined(LTP_DEC)
    ics1->ltp.data_present = faad_get1bit(ld);
#elif defined (DRM)
    if(faad_get1bit(ld)) {
         hInfo->error = 26;
         return;
    }
#else
    faad_get1bit(ld);
#endif

    hInfo->error = side_info(hDecoder, &cpe, ld, ics1, 1);
    if (hInfo->error > 0)
        return;
    if (this_layer_stereo)
    {
        /* Stereo3 */
        ics2->tns_data_present = faad_get1bit(ld);
#ifdef LTP_DEC
        ics1->ltp.data_present =
#endif
            faad_get1bit(ld);
        hInfo->error = side_info(hDecoder, &cpe, ld, ics2, 1);
        if (hInfo->error > 0)
            return;
    }
    /* Stereo4 / Mono2 */
    if (ics1->tns_data_present)
        tns_data(ics1, &(ics1->tns), ld);
    if (this_layer_stereo)
    {
        /* Stereo5 */
        if (ics2->tns_data_present)
            tns_data(ics2, &(ics2->tns), ld);
    }

#ifdef DRM
    /* CRC check */
    if (object_type == DRM_ER_LC)
    {
        if ((hInfo->error = (uint8_t)faad_check_CRC(ld, (uint16_t)faad_get_processed_bits(ld) - 8)) > 0)
            return;
    }
#endif

    /* Stereo6 / Mono3 */
    /* error resilient spectral data decoding */
    if ((hInfo->error = reordered_spectral_data(hDecoder, ics1, ld, spec_data1)) > 0)
    {
        return;
    }
    if (this_layer_stereo)
    {
        /* Stereo7 */
        /* error resilient spectral data decoding */
        if ((hInfo->error = reordered_spectral_data(hDecoder, ics2, ld, spec_data2)) > 0)
        {
            return;
        }
    }


#ifdef DRM
#ifdef SBR_DEC
    /* In case of DRM we need to read the SBR info before channel reconstruction */
    if ((sbr_present_flag == 1) && (object_type == DRM_ER_LC))
    {
        bitfile ld_sbr = {0};
        uint32_t i;
        uint16_t count = 0;
        uint8_t *revbuffer;
        uint8_t *prevbufstart;
        uint8_t *pbufend;

        /* all forward bitreading should be finished at this point */
        uint32_t bitsconsumed = faad_get_processed_bits(ld);
        uint32_t buffer_size = faad_origbitbuffer_size(ld);
        uint8_t *buffer = (uint8_t*)faad_origbitbuffer(ld);

        if (bitsconsumed + 8 > buffer_size*8)
        {
            hInfo->error = 14;
            return;
        }

        if (!sbr[0])
        {
            sbr[0] = sbrDecodeInit(frameLength, element_id[0],
                2*get_sample_rate(sf_index), 0 /* ds SBR */, 1);
        }

        /* Reverse bit reading of SBR data in DRM audio frame */
        revbuffer = (uint8_t*)faad_malloc(buffer_size*sizeof(uint8_t));
        prevbufstart = revbuffer;
        pbufend = &buffer[buffer_size - 1];
        for (i = 0; i < buffer_size; i++)
            *prevbufstart++ = tabFlipbits[*pbufend--];

        /* Set SBR data */
        /* consider 8 bits from AAC-CRC */
        /* SBR buffer size is original buffer size minus AAC buffer size */
        count = (uint16_t)bit2byte(buffer_size*8 - bitsconsumed);
        faad_initbits(&ld_sbr, revbuffer, count);

        sbr[0]->sample_rate = get_sample_rate(sf_index);
        sbr[0]->sample_rate *= 2;

        faad_getbits(&ld_sbr, 8); /* Skip 8-bit CRC */

        sbr[0]->ret = sbr_extension_data(&ld_sbr, sbr[0], count, postSeekResetFlag);
#if (defined(PS_DEC) || defined(DRM_PS))
        if (sbr[0]->ps_used)
        {
            ps_used[0] = 1;
            ps_used_global = 1;
        }
#endif

        if (ld_sbr.error)
        {
            sbr[0]->ret = 1;
        }

        /* check CRC */
        /* no need to check it if there was already an error */
        if (sbr[0]->ret == 0)
            sbr[0]->ret = (uint8_t)faad_check_CRC(&ld_sbr, (uint16_t)faad_get_processed_bits(&ld_sbr) - 8);

        /* SBR data was corrupted, disable it until the next header */
        if (sbr[0]->ret != 0)
        {
            sbr[0]->header_count = 0;
        }

        faad_endbits(&ld_sbr);

        if (revbuffer)
            faad_free(revbuffer);
    }
#endif
#endif

    if (this_layer_stereo)
    {
        hInfo->error = reconstruct_channel_pair(hDecoder, ics1, ics2, &cpe, spec_data1, spec_data2);
        if (hInfo->error > 0)
            return;
    } else {
        hInfo->error = reconstruct_single_channel(hDecoder, ics1, &cpe, spec_data1);
        if (hInfo->error > 0)
            return;
    }

    /* map output channels position to internal data channels */
    if (element_output_channels[fr_ch_ele] == 2)
    {
        /* this might be faulty when pce_set is true */
        internal_channel[channels] = channels;
        internal_channel[channels+1] = channels+1;
    } else {
        internal_channel[channels] = channels;
    }

    fr_channels += element_output_channels[fr_ch_ele];
    fr_ch_ele++;

    return;
}

int8_t DRM_aac_scalable_main_header(ic_stream *ics1, ic_stream *ics2,
                                           bitfile *ld, uint8_t this_layer_stereo)
{
    uint8_t retval = 0;
    uint8_t ch;
    ic_stream *ics;
    uint8_t ics_reserved_bit;

    ics_reserved_bit = faad_get1bit(ld
        DEBUGVAR(1,300,"aac_scalable_main_header(): ics_reserved_bits"));
    if (ics_reserved_bit != 0)
        return 32;
    ics1->window_sequence = (uint8_t)faad_getbits(ld, 2
        DEBUGVAR(1,301,"aac_scalable_main_header(): window_sequence"));
    ics1->window_shape = faad_get1bit(ld
        DEBUGVAR(1,302,"aac_scalable_main_header(): window_shape"));

    if (ics1->window_sequence == EIGHT_SHORT_SEQUENCE)
    {
        ics1->max_sfb = (uint8_t)faad_getbits(ld, 4
            DEBUGVAR(1,303,"aac_scalable_main_header(): max_sfb (short)"));
        ics1->scale_factor_grouping = (uint8_t)faad_getbits(ld, 7
            DEBUGVAR(1,304,"aac_scalable_main_header(): scale_factor_grouping"));
    } else {
        ics1->max_sfb = (uint8_t)faad_getbits(ld, 6
            DEBUGVAR(1,305,"aac_scalable_main_header(): max_sfb (long)"));
    }

    /* get the grouping information */
    if ((retval = window_grouping_info(hDecoder, ics1)) > 0)
        return retval;

    /* should be an error */
    /* check the range of max_sfb */
    if (ics1->max_sfb > ics1->num_swb)
        return 16;

    if (this_layer_stereo)
    {
        ics1->ms_mask_present = (uint8_t)faad_getbits(ld, 2
            DEBUGVAR(1,306,"aac_scalable_main_header(): ms_mask_present"));
        if (ics1->ms_mask_present == 3)
        {
            /* bitstream error */
            return 32;
        }
        if (ics1->ms_mask_present == 1)
        {
            uint8_t g, sfb;
            for (g = 0; g < ics1->num_window_groups; g++)
            {
                for (sfb = 0; sfb < ics1->max_sfb; sfb++)
                {
                    ics1->ms_used[g][sfb] = faad_get1bit(ld
                        DEBUGVAR(1,307,"aac_scalable_main_header(): faad_get1bit"));
                }
            }
        }

        memcpy(ics2, ics1, sizeof(ic_stream));
    } else {
        ics1->ms_mask_present = 0;
    }

    return 0;
}

void* aac_frame_decode(NeAACDecFrameInfo* hInfo,unsigned char * buffer,unsigned long buffer_size)
{
    uint16_t i;
    uint8_t channels = 0;
    uint8_t output_channels = 0;
    bitfile ld = {0};
    uint32_t bitsconsumed;
    uint16_t frame_len;
    void *sample_buffer;
    uint32_t startbit=0, endbit=0, payload_bits=0;

    /* safety checks */
    if ((hInfo == NULL) || (buffer == NULL))
    {
        return NULL;
    }

    frame_len = frameLength;

    memset(hInfo, 0, sizeof(NeAACDecFrameInfo));
    memset(internal_channel, 0, MAX_CHANNELS*sizeof(internal_channel[0]));

    /* initialize the bitstream */
    faad_initbits(&ld, buffer, buffer_size);

    if (object_type == DRM_ER_LC)
    {
        /* We do not support stereo right now */
        if (0) //(channelConfiguration == 2)
        {
            hInfo->error = 28; // Throw CRC error
            goto error;
        }

        faad_getbits(&ld, 8
            DEBUGVAR(1,1,"NeAACDecDecode(): skip CRC"));
    }

    if (adts_header_present)
    {
        adts_header adts;

        adts.old_format = useOldADTSFormat;
        if ((hInfo->error = adts_frame(&adts, &ld)) > 0)
            goto error;

        /* MPEG2 does byte_alignment() here,
         * but ADTS header is always multiple of 8 bits in MPEG2
         * so not needed to actually do it.
         */
    }

    /* decode the complete bitstream */
    if ((object_type == DRM_ER_LC))
    {
        DRM_aac_scalable_main_element(hDecoder, hInfo, &ld, &pce, drc);
    } else {
        raw_data_block(hDecoder, hInfo, &ld, &pce, drc);
    }

    channels = fr_channels;

    if (hInfo->error > 0)
        goto error;

    /* safety check */
    if (channels == 0 || channels > MAX_CHANNELS)
    {
        /* invalid number of channels */
        hInfo->error = 12;
        goto error;
    }

    /* no more bit reading after this */
    bitsconsumed = faad_get_processed_bits(&ld);
    hInfo->bytesconsumed = bit2byte(bitsconsumed);
    if (ld.error)
    {
        hInfo->error = 14;
        goto error;
    }
    faad_endbits(&ld);


    if (!adts_header_present && !adif_header_present)
    {
        if (channelConfiguration == 0)
            channelConfiguration = channels;

        if (channels == 8) /* 7.1 */
			channelConfiguration = 7;
        if (channels == 7) /* not a standard channelConfiguration */
            channelConfiguration = 0;
    }

    if ((channels == 5 || channels == 6) && downMatrix)
    {
        downMatrix = 1;
        output_channels = 2;
    } else {
        output_channels = channels;
    }

    upMatrix = 0;
    /* check if we have a mono file */
    if (output_channels == 1)
    {
        /* upMatrix to 2 channels for implicit signalling of PS */
        upMatrix = 1;
        output_channels = 2;
    }

    /* Make a channel configuration based on either a PCE or a channelConfiguration */
    create_channel_config(hDecoder, hInfo);

    /* number of samples in this frame */
    hInfo->samples = frame_len*output_channels;
    /* number of channels in this frame */
    hInfo->channels = output_channels;
    /* samplerate */
    hInfo->samplerate = get_sample_rate(sf_index);
    /* object type */
    hInfo->object_type = object_type;
    /* sbr */
    hInfo->sbr = NO_SBR;
    /* header type */
    hInfo->header_type = RAW;
    if (adif_header_present)
        hInfo->header_type = ADIF;
    if (adts_header_present)
        hInfo->header_type = ADTS;
    hInfo->ps = ps_used_global;

    /* check if frame has channel elements */
    if (channels == 0)
    {
        frame++;
        return NULL;
    }

    /* allocate the buffer for the final samples */
    if ((sample_buffer == NULL) || (alloced_channels != output_channels))
    {
        static const uint8_t str[] = { sizeof(int16_t), sizeof(int32_t), sizeof(int32_t),
            sizeof(float32_t), sizeof(double), sizeof(int16_t), sizeof(int16_t),
            sizeof(int16_t), sizeof(int16_t), 0, 0, 0
        };
        uint8_t stride = str[outputFormat-1];
        if (((sbr_present_flag == 1)&&(!downSampledSBR)) || (forceUpSampling == 1))
        {
            stride = 2 * stride;
        }
        /* check if we want to use internal sample_buffer */
        if (sample_buffer_size == 0)
        {
            if (sample_buffer)
                faad_free(sample_buffer);
            sample_buffer = NULL;
            sample_buffer = faad_malloc(frame_len*output_channels*stride);
        } else if (sample_buffer_size < frame_len*output_channels*stride) {
            /* provided sample buffer is not big enough */
            hInfo->error = 27;
            return NULL;
        }
        alloced_channels = output_channels;
    }

    if (sample_buffer_size == 0)
    {
        sample_buffer = sample_buffer;
    } else {
        sample_buffer = *sample_buffer2;
    }

    if ((sbr_present_flag == 1) || (forceUpSampling == 1))
    {
        uint8_t ele;

        /* this data is different when SBR is used or when the data is upsampled */
        if (!downSampledSBR)
        {
            frame_len *= 2;
            hInfo->samples *= 2;
            hInfo->samplerate *= 2;
        }

        /* check if every element was provided with SBR data */
        for (ele = 0; ele < fr_ch_ele; ele++)
        {
            if (sbr[ele] == NULL)
            {
                hInfo->error = 25;
                goto error;
            }
        }

        /* sbr */
        if (sbr_present_flag == 1)
        {
            hInfo->object_type = HE_AAC;
            hInfo->sbr = SBR_UPSAMPLED;
        } else {
            hInfo->sbr = NO_SBR_UPSAMPLED;
        }
        if (downSampledSBR)
        {
            hInfo->sbr = SBR_DOWNSAMPLED;
        }
    }


    sample_buffer = output_to_PCM(hDecoder, time_out, sample_buffer,
        output_channels, frame_len, outputFormat);


    conceal_output(hDecoder, frame_len, output_channels, sample_buffer);

    postSeekResetFlag = 0;

    frame++;
    if (object_type != LD)
    {
        if (frame <= 1)
            hInfo->samples = 0;
    } else {
        /* LD encoders will give lower delay */
        if (frame <= 0)
            hInfo->samples = 0;
    }

    return sample_buffer;

error:


    error_state = ERROR_STATE_INIT;

    /* reset filterbank state */
    for (i = 0; i < MAX_CHANNELS; i++)
    {
        if (fb_intermed[i] != NULL)
        {
            memset(fb_intermed[i], 0, frameLength*sizeof(real_t));
        }
    }
    for (i = 0; i < MAX_SYNTAX_ELEMENTS; i++)
    {
        if (sbr[i] != NULL)
        {
            sbrReset(sbr[i]);
        }
    }

    faad_endbits(&ld);

    return NULL;
}


void aac_super_frame_header(int n)
{
}
void audio_frame()
{
}
void aac_crc_bits()
{
}

    unsigned char object_type;
    unsigned char aacSectionDataResilienceFlag;
    unsigned char aacScalefactorDataResilienceFlag;
    unsigned char aacSpectralDataResilienceFlag;
    int frameLength;
    uint8_t sf_index;
    unsigned char channelConfiguration;
	unsigned char sbr_present_flag;

    unsigned char defObjectType;
    unsigned long defSampleRate;
    unsigned char outputFormat;
    unsigned char downMatrix;
    unsigned char useOldADTSFormat;
    unsigned char dontUpSampleImplicitSBR;
};

NeAACDecHandle NEAACDECAPI NeAACDecOpenDummy(void) { return new AACCRCDecoder(); }
void NEAACDECAPI NeAACDecCloseDummy(NeAACDecHandle h) { delete reinterpret_cast<AACCRCDecoder*>(h); }

char NEAACDECAPI NeAACDecInitDRMDummy(NeAACDecHandle* h, unsigned long samplerate, unsigned char channels)
{
	AACCRCDecoder** hDecoder = reinterpret_cast<AACCRCDecoder**>(h);
    if (hDecoder == NULL)
        return 1; /* error */
    delete *hDecoder;
    *hDecoder = new AACCRCDecoder();
	return (*hDecoder)->Init(samplerate, channels);
}

void* NEAACDECAPI NeAACDecDecodeDummy(NeAACDecHandle h,NeAACDecFrameInfo* info,unsigned char * buffer,unsigned long buffer_size)
{
	AACCRCDecoder* dec = reinterpret_cast<AACCRCDecoder*>(h);
    return dec->aac_frame_decode(info, buffer, buffer_size);
}

/* Implementation *************************************************************/
void
CAudioSourceDecoder::ProcessDataInternal(CParameter & ReceiverParam)
{
	int i, j;
	_BOOLEAN bCurBlockOK;
	_BOOLEAN bGoodValues;

	NeAACDecFrameInfo DecFrameInfo;
	short *psDecOutSampleBuf;

	bGoodValues = FALSE;

	ReceiverParam.Lock();
	ReceiverParam.vecbiAudioFrameStatus.Init(0);
	ReceiverParam.vecbiAudioFrameStatus.ResetBitAccess();
	ReceiverParam.Unlock();

	/* Check if something went wrong in the initialization routine */
	if (DoNotProcessData == TRUE)
	{
		return;
	}

	/* Text Message ********************************************************** */
	/* Total frame size depends on whether text message is used or not */
	if (bTextMessageUsed == TRUE)
	{
		/* Decode last for bytes of input block for text message */
		for (i = 0; i < SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
			vecbiTextMessBuf[i] = (*pvecInputData)[iTotalFrameSize + i];

		TextMessage.Decode(vecbiTextMessBuf);
	}

	/* Audio data header parsing ********************************************* */
	/* Check if audio shall not be decoded */
	if (DoNotProcessAudDecoder == TRUE)
	{
		return;
	}

	/* Reset bit extraction access */
	(*pvecInputData).ResetBitAccess();

	/* Check which audio coding type is used */
	if (eAudioCoding == CAudioParam::AC_AAC)
	{
		/* AAC super-frame-header ------------------------------------------- */
		int iPrevBorder = 0;
		for (i = 0; i < iNumBorders; i++)
		{
			/* Frame border in bytes (12 bits) */
			const int iFrameBorder = (*pvecInputData).Separate(12);

			/* The lenght is difference between borders */
			veciFrameLength[i] = iFrameBorder - iPrevBorder;
			iPrevBorder = iFrameBorder;
		}

		/* Byte-alignment (4 bits) in case of 10 audio frames */
		if (iNumBorders == 9)
			(*pvecInputData).Separate(4);

		/* Frame length of last frame */
		veciFrameLength[iNumBorders] = iAudioPayloadLen - iPrevBorder;

		/* Check if frame length entries represent possible values */
		bGoodValues = TRUE;
		for (i = 0; i < iNumAudioFrames; i++)
		{
			if ((veciFrameLength[i] < 0) ||
				(veciFrameLength[i] > iMaxLenOneAudFrame))
			{
				bGoodValues = FALSE;
			}
		}

		if (bGoodValues == TRUE)
		{
			/* Higher-protected part */
			for (i = 0; i < iNumAudioFrames; i++)
			{
				/* Extract higher protected part bytes (8 bits per byte) */
				for (j = 0; j < iNumHigherProtectedBytes; j++)
					audio_frame[i][j] = _BINARY((*pvecInputData).Separate(8));

				/* Extract CRC bits (8 bits) */
				aac_crc_bits[i] = _BINARY((*pvecInputData).Separate(8));
			}

			/* Lower-protected part */
			for (i = 0; i < iNumAudioFrames; i++)
			{
				/* First calculate frame length, derived from higher protected
				   part frame length and total size */
				const int iNumLowerProtectedBytes =
					veciFrameLength[i] - iNumHigherProtectedBytes;

				/* Extract lower protected part bytes (8 bits per byte) */
				for (j = 0; j < iNumLowerProtectedBytes; j++)
				{
					audio_frame[i][iNumHigherProtectedBytes + j] =
						_BINARY((*pvecInputData).Separate(8));
				}
			}
		}
	}
	else if (eAudioCoding == CAudioParam::AC_CELP)
	{
		/* celp_super_frame(celp_table_ind) --------------------------------- */
		/* Higher-protected part */
		for (i = 0; i < iNumAudioFrames; i++)
		{
			celp_frame[i].ResetBitAccess();

			/* Extract higher protected part bits */
			for (j = 0; j < iNumHigherProtectedBits; j++)
				celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);

			/* Extract CRC bits (8 bits) if used */
			if (bCELPCRC == TRUE)
				celp_crc_bits[i] = _BINARY((*pvecInputData).Separate(8));
		}

		/* Lower-protected part */
		for (i = 0; i < iNumAudioFrames; i++)
		{
			for (j = 0; j < iNumLowerProtectedBits; j++)
				celp_frame[i].Enqueue((*pvecInputData).Separate(1), 1);
		}
	}

	/* Audio decoding ******************************************************** */
	/* Init output block size to zero, this variable is also used for
	   determining the position for writing the output vector */
	iOutputBlockSize = 0;

	for (j = 0; j < iNumAudioFrames; j++)
	{
		if (eAudioCoding == CAudioParam::AC_AAC)
		{
			if (bGoodValues == TRUE)
			{
				/* Prepare data vector with CRC at the beginning (the definition
				   with faad2 DRM interface) */
				vecbyPrepAudioFrame[0] = aac_crc_bits[j];

				for (i = 0; i < veciFrameLength[j]; i++)
					vecbyPrepAudioFrame[i + 1] = audio_frame[j][i];

#if 0
// Store AAC-data in file
				string strAACTestFileName = "test/aac_";
	ReceiverParam.Lock();
				if (ReceiverParam.
					Service[ReceiverParam.GetCurSelAudioService()].AudioParam.
					eAudioSamplRate == CAudioParam::AS_12KHZ)
				{
					strAACTestFileName += "12kHz_";
				}
				else
					strAACTestFileName += "24kHz_";

				switch (ReceiverParam.
						Service[ReceiverParam.GetCurSelAudioService()].
						AudioParam.eAudioMode)
				{
				case CAudioParam::AM_MONO:
					strAACTestFileName += "mono";
					break;

				case CAudioParam::AM_P_STEREO:
					strAACTestFileName += "pstereo";
					break;

				case CAudioParam::AM_STEREO:
					strAACTestFileName += "stereo";
					break;
				}

				if (ReceiverParam.
					Service[ReceiverParam.GetCurSelAudioService()].AudioParam.
					eSBRFlag == CAudioParam::SB_USED)
				{
					strAACTestFileName += "_sbr";
				}
				ReceiverParam.Unlock();
				strAACTestFileName += ".dat";
				static FILE *pFile2 = fopen(strAACTestFileName.c_str(), "wb");
				int iNewFrL = veciFrameLength[j] + 1;
				fwrite((void *) &iNewFrL, size_t(4), size_t(1), pFile2);	// frame length
				fwrite((void *) &vecbyPrepAudioFrame[0], size_t(1), size_t(iNewFrL), pFile2);	// data
				fflush(pFile2);
#endif

				/* Call decoder routine */
				psDecOutSampleBuf = (short *) NeAACDecDecode(HandleAACDecoder,
															 &DecFrameInfo,
															 &vecbyPrepAudioFrame
															 [0],
															 veciFrameLength
															 [j] + 1);

				/* OPH: add frame status to vector for RSCI */
				ReceiverParam.Lock();
				ReceiverParam.vecbiAudioFrameStatus.Add(DecFrameInfo.error == 0 ? 0 : 1);
				ReceiverParam.Unlock();
				if (DecFrameInfo.error != 0)
				{
					//cerr << "AAC decode error" << endl;
					bCurBlockOK = FALSE;	/* Set error flag */
				}
				else
				{
					bCurBlockOK = TRUE;

					/* Conversion from _SAMPLE vector to _REAL vector for
					   resampling. ATTENTION: We use a vector which was
					   allocated inside the AAC decoder! */
					if (DecFrameInfo.channels == 1)
					{
						/* Change type of data (short -> real) */
						for (i = 0; i < iLenDecOutPerChan; i++)
							vecTempResBufInLeft[i] = psDecOutSampleBuf[i];

						/* Resample data */
						ResampleObjL.Resample(vecTempResBufInLeft,
											  vecTempResBufOutCurLeft);

						/* Mono (write the same audio material in both
						   channels) */
						for (i = 0; i < iResOutBlockSize; i++)
						{
							vecTempResBufOutCurRight[i] =
								vecTempResBufOutCurLeft[i];
						}
					}
					else
					{
						/* Stereo */
						for (i = 0; i < iLenDecOutPerChan; i++)
						{
							vecTempResBufInLeft[i] = psDecOutSampleBuf[i * 2];
							vecTempResBufInRight[i] =
								psDecOutSampleBuf[i * 2 + 1];
						}

						/* Resample data */
						ResampleObjL.Resample(vecTempResBufInLeft,
											  vecTempResBufOutCurLeft);
						ResampleObjR.Resample(vecTempResBufInRight,
											  vecTempResBufOutCurRight);
					}
				}
			}
			else
			{
				/* DRM AAC header was wrong, set flag to "bad block" */
				bCurBlockOK = FALSE;
				/* OPH: update audio status vector for RSCI */
				ReceiverParam.Lock();
				ReceiverParam.vecbiAudioFrameStatus.Add(1);
				ReceiverParam.Unlock();
			}
		}
		else if (eAudioCoding == CAudioParam::AC_CELP)
		{
			if (bCELPCRC == TRUE)
			{
				/* Prepare CRC object and data stream */
				CELPCRCObject.Reset(8);
				celp_frame[j].ResetBitAccess();

				for (i = 0; i < iNumHigherProtectedBits; i++)
					CELPCRCObject.AddBit((_BINARY) celp_frame[j].Separate(1));

				bCurBlockOK = CELPCRCObject.CheckCRC(celp_crc_bits[j]);
			}
			else
				bCurBlockOK = TRUE;

			/* OPH: update audio status vector for RSCI */
			ReceiverParam.Lock();
			ReceiverParam.vecbiAudioFrameStatus.Add(bCurBlockOK == TRUE ? 0 : 1);
			ReceiverParam.Unlock();

#if 0
// Store CELP-data in file
			char cDummy[200];
			string strCELPTestFileName = "test/celp_";
			ReceiverParam.Lock();
			if (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].
				AudioParam.eAudioSamplRate == CAudioParam::AS_8_KHZ)
			{
				strCELPTestFileName += "8kHz_";
				strCELPTestFileName +=
					_itoa(iTableCELP8kHzUEPParams
						  [ReceiverParam.
						   Service[ReceiverParam.GetCurSelAudioService()].
						   AudioParam.iCELPIndex][0], cDummy, 10);
			}
			else
			{
				strCELPTestFileName += "16kHz_";
				strCELPTestFileName +=
					_itoa(iTableCELP16kHzUEPParams
						  [ReceiverParam.
						   Service[ReceiverParam.GetCurSelAudioService()].
						   AudioParam.iCELPIndex][0], cDummy, 10);
			}
			strCELPTestFileName += "bps";

			if (ReceiverParam.Service[ReceiverParam.GetCurSelAudioService()].
				AudioParam.eSBRFlag == CAudioParam::SB_USED)
			{
				strCELPTestFileName += "_sbr";
			}
			strCELPTestFileName += ".dat";
			ReceiverParam.Unlock();

			static FILE *pFile2 = fopen(strCELPTestFileName.c_str(), "wb");
			int iTotNumBits =
				iNumHigherProtectedBits + iNumLowerProtectedBits;
			int iNewFrL = (int) Ceil((CReal) iTotNumBits / 8);
			fwrite((void *) &iNewFrL, size_t(4), size_t(1), pFile2);	// frame length
			celp_frame[j].ResetBitAccess();
			for (i = 0; i < iNewFrL; i++)
			{
				int iNumBits = Min(iTotNumBits - i * 8, 8);
				_BYTE bCurVal = (_BYTE) celp_frame[j].Separate(iNumBits);
				fwrite((void *) &bCurVal, size_t(1), size_t(1), pFile2);	// data
			}
			fflush(pFile2);
#endif

#ifdef USE_CELP_DECODER

/* Write zeros in current output buffer since we do not have a decoder */
			for (i = 0; i < iResOutBlockSize; i++)
			{
				vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
				vecTempResBufOutCurRight[i] = (_REAL) 0.0;
			}

#endif

		}
		else
			bCurBlockOK = FALSE;

// This code is independent of particular audio source type and should work
// fine with CELP and HVXC

		/* Postprocessing of audio blocks, status informations -------------- */
		if (bCurBlockOK == FALSE)
		{
			if (bAudioWasOK == TRUE)
			{
				/* Post message to show that CRC was wrong (yellow light) */
				ReceiverParam.Lock();
				ReceiverParam.ReceiveStatus.Audio.SetStatus(DATA_ERROR);
				ReceiverParam.ReceiveStatus.LLAudio.SetStatus(DATA_ERROR);
				ReceiverParam.Unlock();

				/* Fade-out old block to avoid "clicks" in audio. We use linear
				   fading which gives a log-fading impression */
				for (i = 0; i < iResOutBlockSize; i++)
				{
					/* Linear attenuation with time of OLD buffer */
					const _REAL rAtt =
						(_REAL) 1.0 - (_REAL) i / iResOutBlockSize;

					vecTempResBufOutOldLeft[i] *= rAtt;
					vecTempResBufOutOldRight[i] *= rAtt;

					if (bUseReverbEffect == TRUE)
					{
						/* Fade in input signal for reverberation to avoid
						   clicks */
						const _REAL rAttRev = (_REAL) i / iResOutBlockSize;

						/* Cross-fade reverberation effect */
						const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
							ProcessSample(vecTempResBufOutOldLeft[i] *
										  rAttRev,
										  vecTempResBufOutOldRight[i] *
										  rAttRev);

						/* Mono reverbration signal */
						vecTempResBufOutOldLeft[i] += rRevSam;
						vecTempResBufOutOldRight[i] += rRevSam;
					}
				}

				/* Set flag to show that audio block was bad */
				bAudioWasOK = FALSE;
			}
			else
			{
				ReceiverParam.Lock();
				ReceiverParam.ReceiveStatus.Audio.SetStatus(CRC_ERROR);
				ReceiverParam.ReceiveStatus.LLAudio.SetStatus(CRC_ERROR);
				ReceiverParam.Unlock();

				if (bUseReverbEffect == TRUE)
				{
					/* Add Reverberation effect */
					for (i = 0; i < iResOutBlockSize; i++)
					{
						/* Mono reverberation signal */
						vecTempResBufOutOldLeft[i] =
							vecTempResBufOutOldRight[i] = AudioRev.
							ProcessSample(0, 0);
					}
				}
			}

			/* Write zeros in current output buffer */
			for (i = 0; i < iResOutBlockSize; i++)
			{
				vecTempResBufOutCurLeft[i] = (_REAL) 0.0;
				vecTempResBufOutCurRight[i] = (_REAL) 0.0;
			}
		}
		else
		{
			/* Increment correctly decoded audio blocks counter */
			iNumCorDecAudio++;

			ReceiverParam.Lock();
			ReceiverParam.ReceiveStatus.Audio.SetStatus(RX_OK);
			ReceiverParam.ReceiveStatus.LLAudio.SetStatus(RX_OK);
			ReceiverParam.Unlock();

			if (bAudioWasOK == FALSE)
			{
				if (bUseReverbEffect == TRUE)
				{
					/* Add "last" reverbration only to old block */
					for (i = 0; i < iResOutBlockSize; i++)
					{
						/* Mono reverberation signal */
						vecTempResBufOutOldLeft[i] =
							vecTempResBufOutOldRight[i] = AudioRev.
							ProcessSample(vecTempResBufOutOldLeft[i],
										  vecTempResBufOutOldRight[i]);
					}
				}

				/* Fade-in new block to avoid "clicks" in audio. We use linear
				   fading which gives a log-fading impression */
				for (i = 0; i < iResOutBlockSize; i++)
				{
					/* Linear attenuation with time */
					const _REAL rAtt = (_REAL) i / iResOutBlockSize;

					vecTempResBufOutCurLeft[i] *= rAtt;
					vecTempResBufOutCurRight[i] *= rAtt;

					if (bUseReverbEffect == TRUE)
					{
						/* Cross-fade reverberation effect */
						const _REAL rRevSam = (1.0 - rAtt) * AudioRev.
							ProcessSample(0, 0);

						/* Mono reverberation signal */
						vecTempResBufOutCurLeft[i] += rRevSam;
						vecTempResBufOutCurRight[i] += rRevSam;
					}
				}

				/* Reset flag */
				bAudioWasOK = TRUE;
			}
		}

		/* Conversion from _REAL to _SAMPLE with special function */
		for (i = 0; i < iResOutBlockSize; i++)
		{
			(*pvecOutputData)[iOutputBlockSize + i * 2] = Real2Sample(vecTempResBufOutOldLeft[i]);	/* Left channel */
			(*pvecOutputData)[iOutputBlockSize + i * 2 + 1] = Real2Sample(vecTempResBufOutOldRight[i]);	/* Right channel */
		}

		/* Add new block to output block size ("* 2" for stereo output block) */
		iOutputBlockSize += iResOutBlockSize * 2;

		/* Store current audio block */
		for (i = 0; i < iResOutBlockSize; i++)
		{
			vecTempResBufOutOldLeft[i] = vecTempResBufOutCurLeft[i];
			vecTempResBufOutOldRight[i] = vecTempResBufOutCurRight[i];
		}
	}
}

void
CAudioSourceDecoder::InitInternal(CParameter & ReceiverParam)
{
/*
	Since we use the exception mechanism in this init routine, the sequence of
	the individual initializations is very important!
	Requirement for text message is "stream is used" and "audio service".
	Requirement for AAC decoding are the requirements above plus "audio coding
	is AAC"
*/
	int iCurAudioStreamID;
	int iMaxLenResamplerOutput;
	int iCurSelServ;
	int iAudioSampleRate;

	/* Init error flags and output block size parameter. The output block
	   size is set in the processing routine. We must set it here in case
	   of an error in the initialization, this part in the processing
	   routine is not being called */
	DoNotProcessAudDecoder = FALSE;
	DoNotProcessData = FALSE;
	iOutputBlockSize = 0;

	try
	{

		ReceiverParam.Lock();

		/* Init counter for correctly decoded audio blocks */
		iNumCorDecAudio = 0;

		/* Init "audio was ok" flag */
		bAudioWasOK = TRUE;

		/* Get number of total input bits for this module */
		iInputBlockSize = ReceiverParam.iNumAudioDecoderBits;

		/* Get current selected audio service */
		iCurSelServ = ReceiverParam.GetCurSelAudioService();

		/* Current audio stream ID */
		iCurAudioStreamID =
			ReceiverParam.Service[iCurSelServ].AudioParam.iStreamID;

		/* The requirement for this module is that the stream is used and the
		   service is an audio service. Check it here */
		if ((ReceiverParam.Service[iCurSelServ].  eAudDataFlag != CService::SF_AUDIO) ||
			(iCurAudioStreamID == STREAM_ID_NOT_USED))
		{
			throw CInitErr(ET_ALL);
		}

		/* Init text message application ------------------------------------ */
		switch (ReceiverParam.Service[iCurSelServ].AudioParam.bTextflag)
		{
		case TRUE:
			bTextMessageUsed = TRUE;

			/* Get a pointer to the string */
			TextMessage.Init(&ReceiverParam.Service[iCurSelServ].AudioParam.
							 strTextMessage);

			/* Total frame size is input block size minus the bytes for the text
			   message */
			iTotalFrameSize = iInputBlockSize -
				SIZEOF__BYTE * NUM_BYTES_TEXT_MESS_IN_AUD_STR;

			/* Init vector for text message bytes */
			vecbiTextMessBuf.Init(SIZEOF__BYTE *
								  NUM_BYTES_TEXT_MESS_IN_AUD_STR);
			break;

		case FALSE:
			bTextMessageUsed = FALSE;

			/* All bytes are used for AAC data, no text message present */
			iTotalFrameSize = iInputBlockSize;
			break;
		}

		/* Get audio coding type */
		eAudioCoding =
			ReceiverParam.Service[iCurSelServ].AudioParam.eAudioCoding;

		if (eAudioCoding == CAudioParam::AC_AAC)
		{
			/* Init for AAC decoding ---------------------------------------- */
			int iAACSampleRate, iNumHeaderBytes, iDRMchanMode = DRMCH_MONO;

			/* Length of higher protected part of audio stream */
			const int iLenAudHigh =
				ReceiverParam.Stream[iCurAudioStreamID].iLenPartA;

			/* Set number of AAC frames in a AAC super-frame */
			switch (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioSamplRate)	/* Only 12 kHz and 24 kHz is allowed */
			{
			case CAudioParam::AS_12KHZ:
				iNumAudioFrames = 5;
				iNumHeaderBytes = 6;
				iAACSampleRate = 12000;
				break;

			case CAudioParam::AS_24KHZ:
				iNumAudioFrames = 10;
				iNumHeaderBytes = 14;
				iAACSampleRate = 24000;
				break;

			default:
				/* Some error occurred, throw error */
				throw CInitErr(ET_AUDDECODER);
				break;
			}

			/* Number of borders */
			iNumBorders = iNumAudioFrames - 1;

			/* Number of channels for AAC: Mono, PStereo, Stereo */
			switch (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioMode)
			{
			case CAudioParam::AM_MONO:
				if (ReceiverParam.Service[iCurSelServ].AudioParam.
					eSBRFlag == CAudioParam::SB_USED)
				{
					iDRMchanMode = DRMCH_SBR_MONO;
				}
				else
					iDRMchanMode = DRMCH_MONO;
				break;

			case CAudioParam::AM_P_STEREO:
				/* Low-complexity only defined in SBR mode */
				iDRMchanMode = DRMCH_SBR_PS_STEREO;
				break;

			case CAudioParam::AM_STEREO:
				if (ReceiverParam.Service[iCurSelServ].AudioParam.
					eSBRFlag == CAudioParam::SB_USED)
				{
					iDRMchanMode = DRMCH_SBR_STEREO;
				}
				else
				{
					iDRMchanMode = DRMCH_STEREO;
				}
				break;
			}

			/* In case of SBR, AAC sample rate is half the total sample rate.
			   Length of output is doubled if SBR is used */
			if (ReceiverParam.Service[iCurSelServ].AudioParam.
				eSBRFlag == CAudioParam::SB_USED)
			{
				iAudioSampleRate = iAACSampleRate * 2;
				iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH * 2;
			}
			else
			{
				iAudioSampleRate = iAACSampleRate;
				iLenDecOutPerChan = AUD_DEC_TRANSFROM_LENGTH;
			}

			/* The audio_payload_length is derived from the length of the audio
			   super frame (data_length_of_part_A + data_length_of_part_B)
			   subtracting the audio super frame overhead (bytes used for the
			   audio super frame header() and for the aac_crc_bits)
			   (5.3.1.1, Table 5) */
			iAudioPayloadLen = iTotalFrameSize / SIZEOF__BYTE -
				iNumHeaderBytes - iNumAudioFrames;

			/* Check iAudioPayloadLen value, only positive values make sense */
			if (iAudioPayloadLen < 0)
				throw CInitErr(ET_AUDDECODER);

			/* Calculate number of bytes for higher protected blocks */
			iNumHigherProtectedBytes = (iLenAudHigh - iNumHeaderBytes -
										iNumAudioFrames /* CRC bytes */ ) /
				iNumAudioFrames;

			if (iNumHigherProtectedBytes < 0)
				iNumHigherProtectedBytes = 0;

			/* The maximum length for one audio frame is "iAudioPayloadLen". The
			   regular size will be much shorter since all audio frames share
			   the total size, but we do not know at this time how the data is
			   split in the transmitter source coder */
			iMaxLenOneAudFrame = iAudioPayloadLen;
			audio_frame.Init(iNumAudioFrames, iMaxLenOneAudFrame);

			/* Init vector which stores the data with the CRC at the beginning
			   ("+ 1" for CRC) */
			vecbyPrepAudioFrame.Init(iMaxLenOneAudFrame + 1);

			/* Init storage for CRCs and frame lengths */
			aac_crc_bits.Init(iNumAudioFrames);
			veciFrameLength.Init(iNumAudioFrames);

			/* Init AAC-decoder */
			NeAACDecInitDRM(&HandleAACDecoder, iAACSampleRate,
							(unsigned char) iDRMchanMode);
		}
		else if (eAudioCoding == CAudioParam::AC_CELP)
		{
			/* Init for CELP decoding --------------------------------------- */
			int iCurCelpIdx, iCelpFrameLength;

			/* Set number of AAC frames in a AAC super-frame */
			switch (ReceiverParam.Service[iCurSelServ].AudioParam.eAudioSamplRate)	/* Only 8000 and 16000 is allowed */
			{
			case CAudioParam::AS_8_KHZ:
				/* Check range */
				iCurCelpIdx =
					ReceiverParam.Service[iCurSelServ].AudioParam.iCELPIndex;

				if ((iCurCelpIdx > 0) &&
					(iCurCelpIdx < LEN_CELP_8KHZ_UEP_PARAMS_TAB))
				{
					/* CELP frame length */
					iCelpFrameLength =
						iTableCELP8kHzUEPParams[iCurCelpIdx][1];

					/* Number of bits for lower and higher protected parts */
					iNumHigherProtectedBits =
						iTableCELP8kHzUEPParams[iCurCelpIdx][2];
					iNumLowerProtectedBits =
						iTableCELP8kHzUEPParams[iCurCelpIdx][3];
				}
				else
					throw CInitErr(ET_AUDDECODER);

				/* Set audio sample rate */
				iAudioSampleRate = 8000;
				break;

			case CAudioParam::AS_16KHZ:
				/* Check range */
				iCurCelpIdx =
					ReceiverParam.Service[iCurSelServ].AudioParam.iCELPIndex;

				if ((iCurCelpIdx > 0) &&
					(iCurCelpIdx < LEN_CELP_16KHZ_UEP_PARAMS_TAB))
				{
					/* CELP frame length */
					iCelpFrameLength =
						iTableCELP16kHzUEPParams[iCurCelpIdx][1];

					/* Number of bits for lower and higher protected parts */
					iNumHigherProtectedBits =
						iTableCELP16kHzUEPParams[iCurCelpIdx][2];
					iNumLowerProtectedBits =
						iTableCELP16kHzUEPParams[iCurCelpIdx][3];
				}
				else
					throw CInitErr(ET_AUDDECODER);

				/* Set audio sample rate */
				iAudioSampleRate = 16000;
				break;

			default:
				/* Some error occurred, throw error */
				throw CInitErr(ET_AUDDECODER);
				break;
			}

			/* Check lengths of iNumHigherProtectedBits and
			   iNumLowerProtectedBits for overrun */
			const int iTotalNumCELPBits =
				iNumHigherProtectedBits + iNumLowerProtectedBits;

			if (iTotalNumCELPBits * SIZEOF__BYTE > iTotalFrameSize)
				throw CInitErr(ET_AUDDECODER);

			/* Calculate number of audio frames (one audio super frame is
			   always 400 ms long) */
			iNumAudioFrames = 400 /* ms */  / iCelpFrameLength /* ms */ ;

			/* Set CELP CRC flag */
			bCELPCRC = ReceiverParam.Service[iCurSelServ].AudioParam.bCELPCRC;

			/* Init vectors storing the CELP raw data and CRCs */
			celp_frame.Init(iNumAudioFrames, iTotalNumCELPBits);
			celp_crc_bits.Init(iNumAudioFrames);

// TEST
			iLenDecOutPerChan = 0;

#ifdef USE_CELP_DECODER

// TODO put decoder initialization here

#else
			/* No CELP decoder available */
			throw CInitErr(ET_AUDDECODER);
#endif
		}
		else
		{
			/* Audio codec not supported */
			throw CInitErr(ET_AUDDECODER);
		}

		/* Set number of Audio frames for log file */
		ReceiverParam.iNumAudioFrames = iNumAudioFrames;

		/* Since we do not correct for sample rate offsets here (yet), we do not
		   have to consider larger buffers. An audio frame always corresponds
		   to 400 ms */
		iMaxLenResamplerOutput = (int) ((_REAL) SOUNDCRD_SAMPLE_RATE *
										(_REAL) 0.4 /* 400ms */  *
										2 /* for stereo */ );

		iResOutBlockSize = (int) ((_REAL) iLenDecOutPerChan *
								  SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);

		/* Additional buffers needed for resampling since we need conversation
		   between _REAL and _SAMPLE. We have to init the buffers with
		   zeros since it can happen, that we have bad CRC right at the
		   start of audio blocks */
		vecTempResBufInLeft.Init(iLenDecOutPerChan, (_REAL) 0.0);
		vecTempResBufInRight.Init(iLenDecOutPerChan, (_REAL) 0.0);
		vecTempResBufOutCurLeft.Init(iResOutBlockSize, (_REAL) 0.0);
		vecTempResBufOutCurRight.Init(iResOutBlockSize, (_REAL) 0.0);
		vecTempResBufOutOldLeft.Init(iResOutBlockSize, (_REAL) 0.0);
		vecTempResBufOutOldRight.Init(iResOutBlockSize, (_REAL) 0.0);

		/* Init resample objects */
		ResampleObjL.Init(iLenDecOutPerChan,
						  (_REAL) SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);
		ResampleObjR.Init(iLenDecOutPerChan,
						  (_REAL) SOUNDCRD_SAMPLE_RATE / iAudioSampleRate);

		/* Clear reverberation object */
		AudioRev.Clear();

		/* With this parameter we define the maximum lenght of the output
		   buffer. The cyclic buffer is only needed if we do a sample rate
		   correction due to a difference compared to the transmitter. But for
		   now we do not correct and we could stay with a single buffer
		   Maybe TODO: sample rate correction to avoid audio dropouts */
		iMaxOutputBlockSize = iMaxLenResamplerOutput;

		ReceiverParam.Unlock();
	}

	catch(CInitErr CurErr)
	{
		ReceiverParam.Unlock();

		switch (CurErr.eErrType)
		{
		case ET_ALL:
			/* An init error occurred, do not process data in this module */
			DoNotProcessData = TRUE;
			break;

		case ET_AUDDECODER:
			/* Audio part should not be decdoded, set flag */
			DoNotProcessAudDecoder = TRUE;
			break;

		default:
			DoNotProcessData = TRUE;
		}

		/* In all cases set output size to zero */
		iOutputBlockSize = 0;
	}
}

int
CAudioSourceDecoder::GetNumCorDecAudio()
{
	/* Return number of correctly decoded audio blocks. Reset counter
	   afterwards */
	const int iRet = iNumCorDecAudio;

	iNumCorDecAudio = 0;

	return iRet;
}

CAudioSourceDecoder::CAudioSourceDecoder()
:	bUseReverbEffect(TRUE), AudioRev((CReal) 1.0 /* seconds delay */ )
{
#ifndef USE_FAAD2_LIBRARY
    NeAACDecOpen = NeAACDecOpenDummy;
    NeAACDecInitDRM = NeAACDecInitDRMDummy;
    NeAACDecClose = NeAACDecCloseDummy;
    NeAACDecDecode = NeAACDecDecodeDummy;
# ifdef _WIN32
    hFaaDlib = LoadLibrary(TEXT("faad_drm.dll"));
    if(hFaaDlib)
    {
		NeAACDecOpen = (NeAACDecOpen_t*)GetProcAddress(hFaaDlib, TEXT("NeAACDecOpen"));
		NeAACDecInitDRM = (NeAACDecInitDRM_t*)GetProcAddress(hFaaDlib, TEXT("NeAACDecInitDRM"));
		NeAACDecClose = (NeAACDecClose_t*)GetProcAddress(hFaaDlib, TEXT("NeAACDecClose"));
		NeAACDecDecode = (NeAACDecDecode_t*)GetProcAddress(hFaaDlib, TEXT("NeAACDecDecode"));
    }
# else
#  if defined(__APPLE__)
    hFaaDlib = dlopen("libfaad_drm.dylib", RTLD_LOCAL | RTLD_NOW);
#  else
    hFaaDlib = dlopen("libfaad2_drm.so", RTLD_LOCAL | RTLD_NOW);
#  endif
    if(hFaaDlib)
    {
		cerr << "loaded aac decoder library OK" << endl;
		NeAACDecOpen = (NeAACDecOpen_t*)dlsym(hFaaDlib, "NeAACDecOpen");
		NeAACDecInitDRM = (NeAACDecInitDRM_t*)dlsym(hFaaDlib, "NeAACDecInitDRM");
		NeAACDecClose = (NeAACDecClose_t*)dlsym(hFaaDlib, "NeAACDecClose");
		NeAACDecDecode = (NeAACDecDecode_t*)dlsym(hFaaDlib,"NeAACDecDecode");
    }
    else
    {
		cerr << "No aac decoder library - audio will not be decoded" << endl;
    }
# endif
    if(NeAACDecInitDRM == NULL) // Might be non-DRM version of FAAD2
    {
		NeAACDecInitDRM = NeAACDecInitDRMDummy;
    }
#endif
	/* Open AACEncoder instance */
	HandleAACDecoder = NeAACDecOpen();

	/* Decoder MUST be initialized at least once, therefore do it here in the
	   constructor with arbitrary values to be sure that this is satisfied */
	NeAACDecInitDRM(&HandleAACDecoder, 24000, DRMCH_MONO);
}

CAudioSourceDecoder::~CAudioSourceDecoder()
{
	/* Close decoder handle */
	NeAACDecClose(HandleAACDecoder);
}
