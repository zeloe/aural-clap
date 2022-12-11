// This file is here to demonstrate how to wire a CLAP plugin
// You can use it as a starting point, however if you are implementing a C++
// plugin, I'd encourage you to use the C++ glue layer instead:
// https://github.com/free-audio/clap-helpers/blob/main/include/clap/helpers/plugin.hh

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <clap/clap.h>




static const clap_plugin_descriptor_t s_my_plug_desc = {
   .clap_version = CLAP_VERSION_INIT,
   .id = "de.zeloe.CodeZen-clap",
   .name = "Aural-Clap",
   .vendor = "CodeZen",
   .url = "https://github.com/zeloe",
   .manual_url = "https://github.com/zeloe",
   .support_url = "https://github.com/zeloe",
   .version = "0.0.1",
   .description = "Mid Side Aural Exciter",
   .features = (const char *[]){
      CLAP_PLUGIN_FEATURE_MIXING,
      CLAP_PLUGIN_FEATURE_STEREO,
      NULL
   },
};
//define param ids
enum ParamIDs
{
    pid_MID = 0,
    pid_SIDE = 1,
    pid_CLIP = 2,
    pid_HIGHPASS = 3,
    pid_MIX = 4
    
};

float m_sampleRate;
typedef struct {
   clap_plugin_t                   plugin;
   const clap_host_t              *host;
   const clap_host_latency_t      *hostLatency;
   const clap_host_log_t          *hostLog;
   const clap_host_thread_check_t *hostThreadCheck;
   const clap_host_params_t *hostParams;
   uint32_t latency;
    //variables
    float mid;
    float side;
    float clip;
    float highpasscutoff;
    float mix;
} my_plug_t;

float th;
float g;
float a0;
float a1;
float a2;
float b1;
float b2;
float b [5]  = {0};
float a [5] = {0};
//filter coeffs
void calculateCoeffsFilter (float cutoff,float sampleRate)
{
             th = 2.0 * M_PI * cutoff / sampleRate;
             g = cos(th) / (1.0 + sin(th));
             a0 = (1.0 + g) / 2.0;
             a1 = -((1.0 + g) / 2.0);
             a2 = 0.0;
             b1 = -g;
             b2 = 0.0;
}
//process Filter
float processFilter(float input)
{
    b[0] = input;
    a[0] = b[0] * a0+ b[1] * a1 + b[2] * a2 - a[1] * b1 - a[2] * b2;
    b[2] = b[1];
    b[1] = b[0];
    a[3] = a[2];
    a[2] = a[1];
    a[1] = a[0];
    return a[0];
}
static void my_plug_process_event(my_plug_t *plug, const clap_event_header_t *hdr);

/////////////////////////////
// clap_plugin_audio_ports //
/////////////////////////////

static uint32_t my_plug_audio_ports_count(const clap_plugin_t *plugin, bool is_input) { return 1; }

static bool my_plug_audio_ports_get(const clap_plugin_t    *plugin,
                                    uint32_t                index,
                                    bool                    is_input,
                                    clap_audio_port_info_t *info) {
   if (index > 0)
      return false;
   info->id = 0;
   snprintf(info->name, sizeof(info->name), "%s", "Stereo In");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    
   return true;
}

static const clap_plugin_audio_ports_t s_my_plug_audio_ports = {
   .count = my_plug_audio_ports_count,
   .get = my_plug_audio_ports_get,
};

//////////////////
// clap_latency //
//////////////////

uint32_t my_plug_latency_get(const clap_plugin_t *plugin) {
   my_plug_t *plug = plugin->plugin_data;
   return plug->latency;
}

static const clap_plugin_latency_t s_my_plug_latency = {
   .get = my_plug_latency_get,
};

//////////////////
// clap_params //
//////////////////
// update this to get more plugin parameters in my case 5
uint32_t clap_param_count(const clap_plugin_t *plugin) { return 5; }
//delcare info
bool clap_param_get_info(const clap_plugin_t *plugin, uint32_t param_index,
                            clap_param_info_t *param_info)
{
    switch (param_index)
    {
    case 0: // mid
        param_info->id = pid_MID;
        strncpy(param_info->name, "Mid", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 0.5;
        param_info->min_value = 0.;
        param_info->max_value = 1.0;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        param_info->cookie = NULL;
        break;
            
    case 1: // side
        param_info->id = pid_SIDE;
        strncpy(param_info->name, "Side", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 0.5;
        param_info->min_value = 0;
        param_info->max_value = 1;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        param_info->cookie = NULL;
        break;
            
    case 2: // clip
        param_info->id = pid_CLIP;
        strncpy(param_info->name, "Clip", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 0.;
        param_info->min_value = 0;
        param_info->max_value = 5;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        param_info->cookie = NULL;
        break;
            
    case 3: // highpass
        param_info->id = pid_HIGHPASS;
        strncpy(param_info->name, "HighpassCutoff", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 5000.;
        param_info->min_value = 100;
        param_info->max_value = 16000;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        param_info->cookie = NULL;
        break;
            
    case 4: // mix
        param_info->id = pid_MIX;
        strncpy(param_info->name, "Mix", CLAP_NAME_SIZE);
        param_info->module[0] = 0;
        param_info->default_value = 0.5;
        param_info->min_value = 0;
        param_info->max_value = 1;
        param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
        param_info->cookie = NULL;
        break;
            
        
    default:
        return false;
    }
    return true;
}
// get value
bool clap_param_get_value(const clap_plugin_t *plugin, clap_id param_id, double *value)
{
    my_plug_t *plug = plugin->plugin_data;

    switch (param_id)
    {
    case pid_MID:
        *value = plug->mid;
        return true;
        break;

    case pid_SIDE:
        *value = plug->side;
        return true;
        break;

    case pid_CLIP:
        *value = plug->clip;
        return true;
        break;
            
    case pid_HIGHPASS:
        *value = plug->highpasscutoff;
        return true;
        break;
    
    case pid_MIX:
        *value = plug->mix;
        return true;
        break;
            
    }

    return false;
}
//get number
bool clap_param_value_to_text(const clap_plugin_t *plugin, clap_id param_id, double value,
                                 char *display, uint32_t size)
{
    my_plug_t *plug = plugin->plugin_data;
    switch (param_id)
    {
    case pid_MID:
        snprintf(display, size, "%f", value);
        return true;
        break;
            
    case pid_SIDE:
        snprintf(display, size, "%f", value);
        return true;
        break;
            
    case pid_CLIP:
        snprintf(display, size, "%f", value);
        return true;
        break;
            
    case pid_HIGHPASS:
        snprintf(display, size, "%f", value);
        return true;
        break;
            
    case pid_MIX:
        snprintf(display, size, "%f", value);
        return true;
        break;
            
    }
    return false;
}
bool clap_text_to_value(const clap_plugin_t *plugin, clap_id param_id, const char *display,
                           double *value)
{
    // I'm not going to bother to support this
    return false;
}

void clap_flush(const clap_plugin_t *plugin, const clap_input_events_t *in,
                   const clap_output_events_t *out)
{
    my_plug_t *plug = plugin->plugin_data;
    int s = in->size(in);
    int q;
    for (q = 0; q < s; ++q)
    {
        const clap_event_header_t *hdr = in->get(in, q);

        my_plug_process_event(plug, hdr);
    }
}
static const clap_plugin_params_t s_my_plug_params = {.count = clap_param_count,
                                                      .get_info = clap_param_get_info,
                                                      .get_value = clap_param_get_value,
                                                      .value_to_text = clap_param_value_to_text,
                                                      .text_to_value = clap_text_to_value,
                                                      .flush = clap_flush};

bool clap_state_save(const clap_plugin_t *plugin, const clap_ostream_t *stream)
{
    my_plug_t *plug = plugin->plugin_data;

    // We need to save 2 doubles and an int to save our state plus a version. This is, of course, a
    // terrible implementation of state. You should do better.
    assert(sizeof(float) == 4);
    assert(sizeof(int32_t) == 4);

    int buffersize = 24;
    char buffer[24];

    int32_t version = 1;
    memcpy(buffer, &version, sizeof(int32_t));
    memcpy(buffer + 4, &(plug->mid), sizeof(float));
    memcpy(buffer + 8, &(plug->side), sizeof(float));
    memcpy(buffer + 12, &(plug->clip), sizeof(float));
    memcpy(buffer + 16, &(plug->highpasscutoff), sizeof(float));
    memcpy(buffer + 20, &(plug->mix), sizeof(float));

    int written = 0;
    char *curr = buffer;
    while (written != buffersize)
    {
        int thiswrite = stream->write(stream, curr, buffersize - written);
        if (thiswrite < 0)
            return false;
        curr += thiswrite;
        written += thiswrite;
    }

    return true;
}


bool clap_state_load(const clap_plugin_t *plugin, const clap_istream_t *stream) {
    my_plug_t *plug = plugin->plugin_data;

    int buffersize = 24;
    char buffer[24];

    int read = 0;
    char *curr = buffer;
    while( read != buffersize)
    {
        int thisread = stream->read(stream, curr, buffersize - read);
        if (thisread < 0)
            return false;
        curr += thisread;
        read += thisread;
    }

    int32_t version;
    memcpy(&version, buffer, sizeof(int32_t));
    memcpy(&plug->mid, buffer + 4, sizeof(float));
    memcpy(&plug->side, buffer + 8, sizeof(float));
    memcpy(&plug->clip, buffer + 12, sizeof(float));
    memcpy(&plug->highpasscutoff, buffer + 16, sizeof(float));
    memcpy(&plug->mix, buffer + 20, sizeof(float));

    return true;
}

static const clap_plugin_state_t s_clap_state = {.save = clap_state_save,
                                                    .load = clap_state_load};

/////////////////
// clap_plugin //
/////////////////

static bool my_plug_init(const struct clap_plugin *plugin) {
   my_plug_t *plug = plugin->plugin_data;

   // Fetch host's extensions here
   plug->hostLog = plug->host->get_extension(plug->host, CLAP_EXT_LOG);
   plug->hostThreadCheck = plug->host->get_extension(plug->host, CLAP_EXT_THREAD_CHECK);
   plug->hostLatency = plug->host->get_extension(plug->host, CLAP_EXT_LATENCY);
    plug->mid = 0.5;
    plug->side = 0.5;
    plug->clip = 1.0;
    plug->highpasscutoff = 5000;
    plug->mix = 0.5;
   return true;
}

static void my_plug_destroy(const struct clap_plugin *plugin) {
   my_plug_t *plug = plugin->plugin_data;
   free(plug);
}

static bool my_plug_activate(const struct clap_plugin *plugin,
                             double                    sample_rate,
                             uint32_t                  min_frames_count,
                             uint32_t                  max_frames_count)
{
    m_sampleRate = sample_rate;
   return true;
}

static void my_plug_deactivate(const struct clap_plugin *plugin) {}

static bool my_plug_start_processing(const struct clap_plugin *plugin) { return true; }

static void my_plug_stop_processing(const struct clap_plugin *plugin) {}

static void my_plug_reset(const struct clap_plugin *plugin) {}

static void my_plug_process_event(my_plug_t *plug, const clap_event_header_t *hdr)
{
    if (hdr->space_id == CLAP_CORE_EVENT_SPACE_ID)
    {
        switch (hdr->type)
        {
        case CLAP_EVENT_PARAM_VALUE:
        {
            const clap_event_param_value_t *ev = (const clap_event_param_value_t *)hdr;
            // TODO: handle parameter change
            switch (ev->param_id)
            {
            case pid_MID:
                plug->mid = ev->value;
                break;
                    
            case pid_SIDE:
                plug->side = ev->value;
                break;
                    
            case pid_CLIP:
                plug->clip = ev->value;
                break;
        
            case pid_HIGHPASS:
                plug->highpasscutoff = ev->value;
                break;
            
            case pid_MIX:
                plug->mix = ev->value;
                break;
                    
                    
            }
            break;
        }
        }
    }
}
float current_mid;
float current_side;
float current_mix;
float current_cutoff;
float current_clip;
static clap_process_status my_plug_process(const struct clap_plugin *plugin,
                                           const clap_process_t     *process)
{
   my_plug_t     *plug = plugin->plugin_data;
   const uint32_t nframes = process->frames_count;
   const uint32_t nev = process->in_events->size(process->in_events);
   uint32_t       ev_index = 0;
   uint32_t       next_ev_frame = nev > 0 ? 0 : nframes;
    
    for (uint32_t i = 0; i < nframes;)
    {
        /* handle every events that happrens at the frame "i" */
        while (ev_index < nev && next_ev_frame == i)
        {
            const clap_event_header_t *hdr = process->in_events->get(process->in_events, ev_index);
            if (hdr->time != i)
            {
                next_ev_frame = hdr->time;
                break;
            }

            my_plug_process_event(plug, hdr);
            ++ev_index;

            if (ev_index == nev)
            {
                // we reached the end of the event list
                next_ev_frame = nframes;
                break;
            }
    }
        
        float local_current_mid = plug->mid;
        float local_current_side = plug->side;
        float local_current_mix = plug->mix;
        float local_current_cutoff = plug->highpasscutoff;
        float local_current_clip = plug->clip;
      /* process every samples until the next event and only update if there is a parameter change*/
        if(local_current_mid !=  current_mid || local_current_side != current_side || local_current_mix != current_mix || local_current_cutoff != current_cutoff || local_current_clip != current_clip)
        {
            const float increment_mid = (local_current_mid - current_mid) / nframes ;
            const float increment_side = (local_current_side - current_side) / nframes;
            const float increment_mix = (local_current_mix - current_mix) / nframes;
            const float increment_cutoff =  (local_current_cutoff - current_cutoff) / nframes;
            const float increment_clip =  (local_current_clip - current_clip) / nframes;
           
            for (; i < next_ev_frame; ++i)
            {
                // fetch input samples
                const float in_l = process->audio_inputs[0].data32[0][i];
                const float in_r = process->audio_inputs[0].data32[1][i];
                current_mid += increment_mid;
                current_side += increment_side;
                current_mix += increment_mix;
                current_cutoff += increment_cutoff;
                current_clip += increment_clip;
                calculateCoeffsFilter(current_cutoff, m_sampleRate);
                const float filteringleft = processFilter(in_l);
                const float filertingright = processFilter(in_r);
                const float mid = ((((filteringleft + filertingright) * 0.5) * current_mid));
                const float side = ((((filteringleft - filertingright) * 0.5) * current_side));
                const float out_l = 0.5 * M_PI * (tanh((mid + side) * current_clip)) * current_mix + (1.f-current_mix )*(in_l);
                const float out_r = 0.5 * M_PI * (tanh((mid - side) * current_clip)) * current_mix + (1.f-current_mix )*(in_r);

                // store output samples
                process->audio_outputs[0].data32[0][i] = out_l;
                process->audio_outputs[0].data32[1][i] = out_r;
            }
            current_mid = local_current_mid;
            current_side = local_current_side;
            current_mix = local_current_mix;
            current_cutoff = local_current_cutoff;
            current_clip = local_current_clip;
        }   else
        {
            // don't do anything
            for (; i < next_ev_frame; ++i)
            {
                calculateCoeffsFilter(current_cutoff, m_sampleRate);
                const float in_l = process->audio_inputs[0].data32[0][i];
                const float in_r = process->audio_inputs[0].data32[1][i];
                const float filteringleft = processFilter(in_l);
                const float filertingright = processFilter(in_r);
                const float mid = ((((filteringleft + filertingright) * 0.5) * current_mid));
                const float side = ((((filteringleft - filertingright) * 0.5) * current_side));
                
                const float out_l = 0.5 * M_PI * (tanh((mid + side) * current_clip)) * current_mix + (1.f-current_mix )*(in_l);
                const float out_r = 0.5 * M_PI * (tanh((mid - side) * current_clip)) * current_mix + (1.f-current_mix )*(in_r);
                process->audio_outputs[0].data32[0][i] = out_l;
                process->audio_outputs[0].data32[1][i] = out_r;
            }
        }
            
        
    }
   return CLAP_PROCESS_CONTINUE;
}

static const void *my_plug_get_extension(const struct clap_plugin *plugin, const char *id) {
   if (!strcmp(id, CLAP_EXT_LATENCY))
      return &s_my_plug_latency;
   if (!strcmp(id, CLAP_EXT_AUDIO_PORTS))
      return &s_my_plug_audio_ports;
    if(!strcmp(id, CLAP_EXT_PARAMS))
        return &s_my_plug_params;
    if (!strcmp(id, CLAP_EXT_STATE))
        return &s_clap_state;
   return NULL;
}

static void my_plug_on_main_thread(const struct clap_plugin *plugin) {}

clap_plugin_t *my_plug_create(const clap_host_t *host) {
   my_plug_t *p = calloc(1, sizeof(*p));
   p->host = host;
   p->plugin.desc = &s_my_plug_desc;
   p->plugin.plugin_data = p;
   p->plugin.init = my_plug_init;
   p->plugin.destroy = my_plug_destroy;
   p->plugin.activate = my_plug_activate;
   p->plugin.deactivate = my_plug_deactivate;
   p->plugin.start_processing = my_plug_start_processing;
   p->plugin.stop_processing = my_plug_stop_processing;
   p->plugin.reset = my_plug_reset;
   p->plugin.process = my_plug_process;
   p->plugin.get_extension = my_plug_get_extension;
   p->plugin.on_main_thread = my_plug_on_main_thread;
   
   // Don't call into the host here

   return &p->plugin;
}

/////////////////////////
// clap_plugin_factory //
/////////////////////////

static struct {
   const clap_plugin_descriptor_t *desc;
   clap_plugin_t *(*create)(const clap_host_t *host);
} s_plugins[] = {
   {
      .desc = &s_my_plug_desc,
      .create = my_plug_create,
   },
};

static uint32_t plugin_factory_get_plugin_count(const struct clap_plugin_factory *factory) {
   return sizeof(s_plugins) / sizeof(s_plugins[0]);
}

static const clap_plugin_descriptor_t *
plugin_factory_get_plugin_descriptor(const struct clap_plugin_factory *factory, uint32_t index) {
   return s_plugins[index].desc;
}

static const clap_plugin_t *plugin_factory_create_plugin(const struct clap_plugin_factory *factory,
                                                         const clap_host_t                *host,
                                                         const char *plugin_id) {
   if (!clap_version_is_compatible(host->clap_version)) {
      return NULL;
   }

   const int N = sizeof(s_plugins) / sizeof(s_plugins[0]);
   for (int i = 0; i < N; ++i)
      if (!strcmp(plugin_id, s_plugins[i].desc->id))
         return s_plugins[i].create(host);

   return NULL;
}

static const clap_plugin_factory_t s_plugin_factory = {
   .get_plugin_count = plugin_factory_get_plugin_count,
   .get_plugin_descriptor = plugin_factory_get_plugin_descriptor,
   .create_plugin = plugin_factory_create_plugin,
};

////////////////
// clap_entry //
////////////////

static bool entry_init(const char *plugin_path) {
   // called only once, and very first
   return true;
}

static void entry_deinit(void) {
   // called before unloading the DSO
}

static const void *entry_get_factory(const char *factory_id) {
   if (!strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID))
      return &s_plugin_factory;
   return NULL;
}

CLAP_EXPORT const clap_plugin_entry_t clap_entry = {
   .clap_version = CLAP_VERSION_INIT,
   .init = entry_init,
   .deinit = entry_deinit,
    .get_factory = entry_get_factory
};
