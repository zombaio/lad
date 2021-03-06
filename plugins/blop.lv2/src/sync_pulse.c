/*
  An LV2 plugin to generate a non-bandlimited variable-pulse waveform with gate
  for trigger and sync.
  Copyright 2011 David Robillard
  Copyright 2003 Mike Rawes

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include "lv2/lv2plug.in/ns/ext/morph/morph.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "uris.h"
#include "common.h"

#define SYNCPULSE_FREQUENCY  0
#define SYNCPULSE_PULSEWIDTH 1
#define SYNCPULSE_GATE       2
#define SYNCPULSE_OUTPUT     3

typedef struct {
	const float* frequency;
	const float* pulsewidth;
	const float* gate;
	float*       output;
	float        srate;
	float        phase;
	uint32_t     frequency_is_cv;
	uint32_t     pulsewidth_is_cv;
	URIs         uris;
} SyncPulse;

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	SyncPulse* plugin = (SyncPulse*)instance;

	switch (port) {
	case SYNCPULSE_FREQUENCY:
		plugin->frequency = (const float*)data;
		break;
	case SYNCPULSE_PULSEWIDTH:
		plugin->pulsewidth = (const float*)data;
		break;
	case SYNCPULSE_GATE:
		plugin->gate = (const float*)data;
		break;
	case SYNCPULSE_OUTPUT:
		plugin->output = (float*)data;
		break;
	}
}

static uint32_t
options_set(LV2_Handle                instance,
            const LV2_Options_Option* options)
{
	SyncPulse* plugin = (SyncPulse*)malloc(sizeof(SyncPulse));
	uint32_t   ret    = 0;
	for (const LV2_Options_Option* o = options; o->key; ++o) {
		if (o->context != LV2_OPTIONS_PORT) {
			ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
		} else if (o->key != plugin->uris.morph_currentType) {
			ret |= LV2_OPTIONS_ERR_BAD_KEY;
		} else if (o->type != plugin->uris.atom_URID) {
			ret |= LV2_OPTIONS_ERR_BAD_VALUE;
		} else {
			LV2_URID port_type = *(const LV2_URID*)(o->value);
			if (port_type != plugin->uris.lv2_ControlPort &&
			    port_type != plugin->uris.lv2_CVPort) {
				ret |= LV2_OPTIONS_ERR_BAD_VALUE;
				continue;
			}

			switch (o->subject) {
			case SYNCPULSE_FREQUENCY:
				plugin->frequency_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			case SYNCPULSE_PULSEWIDTH:
				plugin->pulsewidth_is_cv = (port_type == plugin->uris.lv2_CVPort);
				break;
			default:
				ret |= LV2_OPTIONS_ERR_BAD_SUBJECT;
			}
		}
	}
	return ret;
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    sample_rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	SyncPulse* plugin = (SyncPulse*)malloc(sizeof(SyncPulse));

	if (plugin) {
		plugin->srate            = (float)sample_rate;
		plugin->frequency_is_cv  = 0;
		plugin->pulsewidth_is_cv = 0;
		map_uris(&plugin->uris, features);
	}

	return (LV2_Handle)plugin;
}

static void
activate(LV2_Handle instance)
{
	SyncPulse* plugin = (SyncPulse*)instance;

	plugin->phase = 0.0f;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	SyncPulse* plugin = (SyncPulse*)instance;

	/* Frequency (array of float of length sample_count) */
	const float* frequency = plugin->frequency;

	/* Pulse Width (array of float of length sample_count) */
	const float* pulsewidth = plugin->pulsewidth;

	/* Gate (array of float of length sample_count) */
	const float* gate = plugin->gate;

	/* Output (pointer to float value) */
	float* output = plugin->output;

	/* Instance data */
	float phase = plugin->phase;
	float srate = plugin->srate;

	for (uint32_t s = 0; s < sample_count; ++s) {
		if (gate[s] > 0.0f) {
			const float freq   = frequency[s * plugin->frequency_is_cv];
			const float pw     = pulsewidth[s * plugin->pulsewidth_is_cv];
			const float pwidth = f_clip(pw, 0.0f, 1.0f) * srate;

			if (phase < pwidth) {
				output[s] = 1.0f;
			} else {
				output[s] = -1.0f;
			}

			phase += freq;
			if (phase < 0.0f) {
				phase += srate;
			} else if (phase > srate) {
				phase -= srate;
			}
		} else {
			output[s] = 0.0f;
			phase     = 0.0f;
		}
	}

	plugin->phase = phase;
}

static const void*
extension_data(const char* uri)
{
	static const LV2_Options_Interface options = { NULL, options_set };
	if (!strcmp(uri, LV2_OPTIONS__interface)) {
		return &options;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	"http://drobilla.net/plugins/blop/sync_pulse",
	instantiate,
	connect_port,
	activate,
	run,
	NULL,
	cleanup,
	extension_data,
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
