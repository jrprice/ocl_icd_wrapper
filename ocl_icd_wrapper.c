#include <stdio.h>
#include <string.h>

#include "icd_dispatch.h"

// Platform wrapper object
static cl_platform_id m_platform = NULL;

// Function to initialize the dispatch table
KHRicdVendorDispatch* createDispatchTable();

CL_API_ENTRY cl_int CL_API_CALL
clIcdGetPlatformIDsKHR(cl_uint num_entries,
                       cl_platform_id *platforms,
                       cl_uint *num_platforms)
{
  // If first call, initialize platform
  if (!m_platform)
  {
    // Get real platform ID
    cl_platform_id platform;
    cl_int err = clGetPlatformIDs(1, &platform, NULL);
    if (err != CL_SUCCESS)
    {
      return err;
    }

    // Create dispatch table
    KHRicdVendorDispatch *table = createDispatchTable(&table);
    if (!table)
    {
      return CL_OUT_OF_RESOURCES;
    }

    // Create platform object
    m_platform = (cl_platform_id)malloc(sizeof(struct _cl_platform_id));
    m_platform->dispatch = table;
    m_platform->platform = platform;
  }

  if (num_entries > 0)
  {
    platforms[0] = m_platform;
  }

  if (num_platforms)
  {
    *num_platforms = 1;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY void* CL_API_CALL
clGetExtensionFunctionAddress(const char *funcname) CL_API_SUFFIX__VERSION_1_2
{
  if (strcmp(funcname, "clIcdGetPlatformIDsKHR") == 0)
  {
    return (void*)clIcdGetPlatformIDsKHR;
  }
  else
  {
    return NULL;
  }
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetPlatformInfo_(cl_platform_id    platform,
                    cl_platform_info  param_name,
                    size_t            param_value_size,
                    void *            param_value,
                    size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_PLATFORM_ICD_SUFFIX_KHR)
  {
    const char *suffix = "oiw";
    int len = strlen(suffix) + 1;
    if (param_value_size && param_value_size < len)
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, suffix, len);
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = len;
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetPlatformInfo(
      platform->platform,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetDeviceIDs_(cl_platform_id   platform,
                 cl_device_type   device_type,
                 cl_uint          num_entries,
                 cl_device_id *   devices,
                 cl_uint *        num_devices) CL_API_SUFFIX__VERSION_1_0
{
  // Allocate array for real devices
  cl_device_id *_devices = NULL;
  if (num_entries > 0 && devices)
  {
    _devices = malloc(num_entries * sizeof(cl_device_id));
  }

  // Call original function
  cl_uint _num_devices = 0;
  cl_int err = clGetDeviceIDs(
    platform->platform,
    device_type,
    num_entries,
    _devices,
    &_num_devices
  );

  if (devices && err == CL_SUCCESS)
  {
    // Create wrapper object for each real device
    for (int i = 0; i < _num_devices && i < num_entries; i++)
    {
      devices[i] = malloc(sizeof(struct _cl_device_id));
      devices[i]->dispatch = platform->dispatch;
      devices[i]->platform = platform;
      devices[i]->device = _devices[i];
    }
    free(_devices);
  }

  if (num_devices)
  {
    *num_devices = _num_devices;
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetDeviceInfo_(cl_device_id    device,
                  cl_device_info  param_name,
                  size_t          param_value_size,
                  void *          param_value,
                  size_t *        param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_DEVICE_PLATFORM)
  {
    if (param_value_size && param_value_size < sizeof(cl_platform_id))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &device->platform, sizeof(cl_platform_id));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_platform_id);
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetDeviceInfo(
      device->device,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_int CL_API_CALL
_clCreateSubDevices_(cl_device_id in_device,
                     const cl_device_partition_property *properties,
                     cl_uint num_entries,
                     cl_device_id *out_devices,
                     cl_uint *num_devices) CL_API_SUFFIX__VERSION_1_2
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainDevice_(cl_device_id device) CL_API_SUFFIX__VERSION_1_2
{
  return clRetainDevice(device->device);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseDevice_(cl_device_id device) CL_API_SUFFIX__VERSION_1_2
{
  return clReleaseDevice(device->device);
}

// Utility function to create a list of real devices
cl_device_id* createDeviceList(cl_uint num, const cl_device_id *list)
{
  cl_device_id *devices = NULL;
  if (list && num)
  {
    devices = malloc(num*sizeof(cl_device_id));
    for (int i = 0; i < num; i++)
    {
      devices[i] = list[i]->device;
    }
  }
  return devices;
}

// Utility to get the number of entries in a NULL-terminated property list
cl_uint getNumProperties(const cl_context_properties *properties)
{
  if (!properties)
  {
    return 0;
  }
  const cl_context_properties *p = properties;
  while (*p++);
  return p - properties;
}

// Utlity to check for platform property and replace if necessary
cl_context_properties* createContextProperties(const cl_context_properties *properties)
{
  if (!properties)
  {
    return NULL;
  }
  int numProps = getNumProperties(properties);
  cl_context_properties *_properties =
    malloc(numProps*sizeof(cl_context_properties));
  for (int i = 0; i < numProps-1; i+=2)
  {
    _properties[i] = properties[i];
    if (properties[i] == CL_CONTEXT_PLATFORM)
    {
      cl_platform_id platform = (cl_platform_id)properties[i+1];
      _properties[i+1] = (cl_context_properties)platform->platform;
    }
    else
    {
      _properties[i+1] = properties[i+1];
    }
  }
  _properties[numProps-1] = 0;
  return _properties;
}

CL_API_ENTRY cl_context CL_API_CALL
_clCreateContext_(const cl_context_properties * properties,
                  cl_uint                       num_devices ,
                  const cl_device_id *          devices,
                  void (CL_CALLBACK * pfn_notify)(const char *, const void *, size_t, void *),
                  void *                        user_data,
                  cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Create real device array
  cl_device_id *_devices = createDeviceList(num_devices, devices);

  // Call original function
  cl_context_properties *_properties = createContextProperties(properties);
  cl_int err;
  cl_context _context = clCreateContext(
    _properties,
    num_devices,
    _devices,
    pfn_notify,
    user_data,
    &err
  );
  if (_properties)
  {
    free(_properties);
  }

  // Create wrapper object
  cl_context context = NULL;
  if (err == CL_SUCCESS)
  {
    context = malloc(sizeof(struct _cl_context));
    context->dispatch = devices[0]->dispatch;
    context->context = _context;
    context->platform = devices[0]->platform;
    context->numDevices = num_devices;
    context->devices = malloc(num_devices*sizeof(struct _cl_device_id));
    memcpy(context->devices, devices, num_devices*sizeof(struct _cl_device_id));

    if (properties)
    {
      cl_uint num = getNumProperties(properties);
      context->numProperties = num;
      context->properties = malloc(num*sizeof(cl_context_properties));
      memcpy(context->properties, properties, num*sizeof(cl_context_properties));
    }
    else
    {
      context->numProperties = 0;
      context->properties = NULL;
    }
  }

  if (_devices)
  {
    free(_devices);
  }
  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return context;
}

CL_API_ENTRY cl_context CL_API_CALL
_clCreateContextFromType_(const cl_context_properties * properties,
                          cl_device_type                device_type,
                          void (CL_CALLBACK *     pfn_notify)(const char *, const void *, size_t, void *),
                          void *                        user_data,
                          cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Call original function
  cl_context_properties *_properties = createContextProperties(properties);
  cl_int err;
  cl_context _context = clCreateContextFromType(
    _properties,
    device_type,
    pfn_notify,
    user_data,
    &err
  );
  if (_properties)
  {
    free(_properties);
  }

  cl_context context = NULL;
  if (err == CL_SUCCESS)
  {
    // Get real devices
    cl_uint num;
    clGetContextInfo(
      _context,
      CL_CONTEXT_NUM_DEVICES,
      sizeof(cl_uint),
      &num,
      NULL);
    cl_device_id *_devices = malloc(num*sizeof(cl_device_id));
    clGetContextInfo(
      _context,
      CL_CONTEXT_DEVICES,
      num*sizeof(cl_device_id),
      _devices,
      NULL);

    // Create wrapper object
    context = malloc(sizeof(struct _cl_context));
    context->dispatch = m_platform->dispatch;
    context->context = _context;
    context->platform = m_platform;

    if (properties)
    {
      cl_uint num = getNumProperties(properties);
      context->numProperties = num;
      context->properties = malloc(num*sizeof(cl_context_properties));
      memcpy(context->properties, properties, num*sizeof(cl_context_properties));
    }
    else
    {
      context->numProperties = 0;
      context->properties = NULL;
    }

    context->numDevices = num;
    context->devices = malloc(num*sizeof(cl_device_id));
    for (int i = 0; i < num; i++)
    {
      context->devices[i] = malloc(sizeof(struct _cl_device_id));
      context->devices[i]->dispatch = context->dispatch;
      context->devices[i]->device = _devices[i];
      context->devices[i]->platform = m_platform;
    }
    free(_devices);
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return context;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainContext_(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  return clRetainContext(context->context);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseContext_(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  return clReleaseContext(context->context);
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetContextInfo_(cl_context         context,
                   cl_context_info    param_name,
                   size_t             param_value_size,
                   void *             param_value,
                   size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_CONTEXT_DEVICES)
  {
    size_t sz = context->numDevices * sizeof(cl_device_id);
    if (param_value_size && param_value_size < sz)
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, context->devices, sz);
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sz;
    }
    return CL_SUCCESS;
  }
  else if (param_name == CL_CONTEXT_PLATFORM)
  {
    if (param_value_size && param_value_size < sizeof(cl_platform_id))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &context->platform, sizeof(cl_platform_id));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_platform_id);
    }
    return CL_SUCCESS;
  }
  else if (param_name == CL_CONTEXT_PROPERTIES)
  {
    size_t sz = context->numProperties*sizeof(cl_context_properties);
    if (param_value_size && param_value_size < sz)
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, context->properties, sz);
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sz;
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetContextInfo(
      context->context,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_command_queue CL_API_CALL
_clCreateCommandQueue_(cl_context                     context,
                       cl_device_id                   device,
                       cl_command_queue_properties    properties,
                       cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Call original function
  cl_int err;
  cl_command_queue _queue = clCreateCommandQueue(
    context->context,
    device->device,
    properties,
    &err
  );

  // Create wrapper object
  cl_command_queue queue = NULL;
  if (err == CL_SUCCESS)
  {
    queue = malloc(sizeof(struct _cl_command_queue));
    queue->dispatch = context->dispatch;
    queue->queue = _queue;
    queue->context = context;
    queue->device = device;
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return queue;
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetCommandQueueProperty_(cl_command_queue               command_queue ,
                            cl_command_queue_properties    properties ,
                            cl_bool                        enable ,
                            cl_command_queue_properties *  old_properties) //CL_EXT_SUFFIX__VERSION_1_0_DEPRECATED
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainCommandQueue_(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return clRetainCommandQueue(command_queue->queue);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseCommandQueue_(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return clReleaseCommandQueue(command_queue->queue);
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetCommandQueueInfo_(cl_command_queue       command_queue ,
                        cl_command_queue_info  param_name ,
                        size_t                 param_value_size ,
                        void *                 param_value ,
                        size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_QUEUE_CONTEXT)
  {
    if (param_value_size && param_value_size < sizeof(cl_context))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &command_queue->context, sizeof(cl_context));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_context);
    }
    return CL_SUCCESS;
  }
  else if (param_name == CL_QUEUE_DEVICE)
  {
    if (param_value_size && param_value_size < sizeof(cl_device_id))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &command_queue->device, sizeof(cl_device_id));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_device_id);
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetCommandQueueInfo(
      command_queue->queue,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateBuffer_(cl_context    context ,
                 cl_mem_flags  flags ,
                 size_t        size ,
                 void *        host_ptr ,
                 cl_int *      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Call original function
  cl_int err;
  cl_mem _buffer = clCreateBuffer(
    context->context,
    flags,
    size,
    host_ptr,
    &err
  );

  // Create wrapper object
  cl_mem buffer = NULL;
  if (err == CL_SUCCESS)
  {
    buffer = malloc(sizeof(struct _cl_mem));
    buffer->dispatch = context->dispatch;
    buffer->mem = _buffer;
    buffer->context = context;
    buffer->parent = NULL;
    buffer->imgBuffer = NULL;
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return buffer;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateSubBuffer_(cl_mem                    buffer ,
                    cl_mem_flags              flags ,
                    cl_buffer_create_type     buffer_create_type ,
                    const void *              buffer_create_info ,
                    cl_int *                  errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
  // Call original function
  cl_int err;
  cl_mem _subbuffer = clCreateSubBuffer(
    buffer->mem,
    flags,
    buffer_create_type,
    buffer_create_info,
    &err
  );

  // Create wrapper object
  cl_mem subbuffer = NULL;
  if (err == CL_SUCCESS)
  {
    subbuffer = malloc(sizeof(struct _cl_mem));
    subbuffer->dispatch = buffer->dispatch;
    subbuffer->mem = _subbuffer;
    subbuffer->context = buffer->context;
    subbuffer->parent = buffer;
    subbuffer->imgBuffer = NULL;
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return subbuffer;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateImage_(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                const cl_image_desc *   image_desc,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  cl_image_desc _desc = *image_desc;
  if (_desc.image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER && _desc.buffer)
  {
    _desc.buffer = _desc.buffer->mem;
  }

  // Call original function
  cl_int err;
  cl_mem _buffer = clCreateImage(
    context->context,
    flags,
    image_format,
    &_desc,
    host_ptr,
    &err
  );

  // Create wrapper object
  cl_mem buffer = NULL;
  if (err == CL_SUCCESS)
  {
    buffer = malloc(sizeof(struct _cl_mem));
    buffer->dispatch = context->dispatch;
    buffer->mem = _buffer;
    buffer->context = context;
    buffer->parent = NULL;
    buffer->imgBuffer = NULL;
    if (image_desc->image_type == CL_MEM_OBJECT_IMAGE1D_BUFFER)
    {
      buffer->imgBuffer = image_desc->buffer;
    }
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return buffer;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateImage2D_(cl_context              context ,
                  cl_mem_flags            flags ,
                  const cl_image_format * image_format ,
                  size_t                  image_width ,
                  size_t                  image_height ,
                  size_t                  image_row_pitch ,
                  void *                  host_ptr ,
                  cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateImage3D_(cl_context              context,
                  cl_mem_flags            flags,
                  const cl_image_format * image_format,
                  size_t                  image_width,
                  size_t                  image_height ,
                  size_t                  image_depth ,
                  size_t                  image_row_pitch ,
                  size_t                  image_slice_pitch ,
                  void *                  host_ptr ,
                  cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainMemObject_(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
  return clRetainMemObject(memobj->mem);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseMemObject_(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
  return clReleaseMemObject(memobj->mem);
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetSupportedImageFormats_(cl_context           context,
                             cl_mem_flags         flags,
                             cl_mem_object_type   image_type ,
                             cl_uint              num_entries ,
                             cl_image_format *    image_formats ,
                             cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0
{
  return clGetSupportedImageFormats(
    context->context,
    flags,
    image_type,
    num_entries,
    image_formats,
    num_image_formats
  );
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetMemObjectInfo_(cl_mem            memobj ,
                     cl_mem_info       param_name ,
                     size_t            param_value_size ,
                     void *            param_value ,
                     size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_MEM_CONTEXT)
  {
    if (param_value_size && param_value_size < sizeof(cl_context))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &(memobj->context), sizeof(cl_context));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_context);
    }
    return CL_SUCCESS;
  }
  else if (param_name == CL_MEM_ASSOCIATED_MEMOBJECT)
  {
    if (param_value_size && param_value_size < sizeof(cl_mem))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &(memobj->parent), sizeof(cl_mem));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_mem);
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetMemObjectInfo(
      memobj->mem,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetImageInfo_(cl_mem            image ,
                 cl_image_info     param_name ,
                 size_t            param_value_size ,
                 void *            param_value ,
                 size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_IMAGE_BUFFER)
  {
    if (param_value_size && param_value_size < sizeof(cl_mem))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &(image->imgBuffer), sizeof(cl_mem));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_mem);
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetImageInfo(
      image->mem,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetMemObjectDestructorCallback_(cl_mem  memobj ,
                                   void (CL_CALLBACK * pfn_notify)(cl_mem  memobj , void* user_data),
                                   void * user_data)             CL_API_SUFFIX__VERSION_1_1
{
  return clSetMemObjectDestructorCallback(
    memobj->mem,
    pfn_notify,
    user_data
  );
}

CL_API_ENTRY cl_sampler CL_API_CALL
_clCreateSampler_(cl_context           context ,
                  cl_bool              normalized_coords ,
                  cl_addressing_mode   addressing_mode ,
                  cl_filter_mode       filter_mode ,
                  cl_int *             errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Call original function
  cl_int err;
  cl_sampler _sampler = clCreateSampler(
    context->context,
    normalized_coords,
    addressing_mode,
    filter_mode,
    &err
  );

  // Create wrapper object
  cl_sampler sampler = NULL;
  if (err == CL_SUCCESS)
  {
    sampler = malloc(sizeof(struct _cl_sampler));
    sampler->dispatch = context->dispatch;
    sampler->sampler = _sampler;
    sampler->context = context;
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return sampler;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainSampler_(cl_sampler  sampler) CL_API_SUFFIX__VERSION_1_0
{
  return clRetainSampler(sampler->sampler);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseSampler_(cl_sampler  sampler) CL_API_SUFFIX__VERSION_1_0
{
  return clReleaseSampler(sampler->sampler);
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetSamplerInfo_(cl_sampler          sampler ,
                   cl_sampler_info     param_name ,
                   size_t              param_value_size ,
                   void *              param_value ,
                   size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_SAMPLER_CONTEXT)
  {
    if (param_value_size && param_value_size < sizeof(cl_context))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &sampler->context, sizeof(cl_context));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_context);
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetSamplerInfo(
      sampler->sampler,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_program CL_API_CALL
_clCreateProgramWithSource_(cl_context         context ,
                            cl_uint            count ,
                            const char **      strings ,
                            const size_t *     lengths ,
                            cl_int *           errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Call original function
  cl_int err;
  cl_program _program = clCreateProgramWithSource(
    context->context,
    count,
    strings,
    lengths,
    &err
  );

  // Create wrapper object
  cl_program program = NULL;
  if (err == CL_SUCCESS)
  {
    program = malloc(sizeof(struct _cl_program));
    program->dispatch = context->dispatch;
    program->program = _program;
    program->context = context;
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return program;
}

CL_API_ENTRY cl_program CL_API_CALL
_clCreateProgramWithBinary_(cl_context                      context ,
                            cl_uint                         num_devices ,
                            const cl_device_id *            device_list ,
                            const size_t *                  lengths ,
                            const unsigned char **          binaries ,
                            cl_int *                        binary_status ,
                            cl_int *                        errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  cl_device_id *_devices = createDeviceList(num_devices, device_list);

  // Call original function
  cl_int err;
  cl_program _program = clCreateProgramWithBinary(
    context->context,
    num_devices,
    _devices,
    lengths,
    binaries,
    binary_status,
    &err
  );

  // Create wrapper object
  cl_program program = NULL;
  if (err == CL_SUCCESS)
  {
    program = malloc(sizeof(struct _cl_program));
    program->dispatch = context->dispatch;
    program->program = _program;
    program->context = context;
  }

  if (_devices)
  {
    free(_devices);
  }
  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return program;
}

CL_API_ENTRY cl_program CL_API_CALL
_clCreateProgramWithBuiltInKernels_(cl_context             context ,
                                    cl_uint                num_devices ,
                                    const cl_device_id *   device_list ,
                                    const char *           kernel_names ,
                                    cl_int *               errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  cl_device_id *_devices = createDeviceList(num_devices, device_list);

  // Call original function
  cl_int err;
  cl_program _program = clCreateProgramWithBuiltInKernels(
    context->context,
    num_devices,
    _devices,
    kernel_names,
    &err
  );

  // Create wrapper object
  cl_program program = NULL;
  if (err == CL_SUCCESS)
  {
    program = malloc(sizeof(struct _cl_program));
    program->dispatch = context->dispatch;
    program->program = _program;
    program->context = context;
  }

  if (_devices)
  {
    free(_devices);
  }
  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return program;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainProgram_(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  return clRetainProgram(program->program);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseProgram_(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  return clReleaseProgram(program->program);
}

CL_API_ENTRY cl_int CL_API_CALL
_clBuildProgram_(cl_program            program ,
                 cl_uint               num_devices ,
                 const cl_device_id *  device_list ,
                 const char *          options ,
                 void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
                 void *                user_data) CL_API_SUFFIX__VERSION_1_0
{
  cl_device_id *_devices = createDeviceList(num_devices, device_list);

  const char *_options = options ? options : "";
  size_t sz = strlen(_options) + strlen("-cl-kernel-arg-info") + 2;
  char *buildOptions = malloc(sz);
  sprintf(buildOptions, "%s -cl-kernel-arg-info", _options);
  cl_uint err = clBuildProgram(
    program->program,
    num_devices,
    _devices,
    buildOptions,
    pfn_notify,
    user_data
  );
  free(buildOptions);

  if (_devices)
  {
    free(_devices);
  }
  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clUnloadCompiler_(void) CL_API_SUFFIX__VERSION_1_0
{
  return clUnloadCompiler();
}

cl_program* createProgramList(cl_uint num, const cl_program *list)
{
  cl_program *programs = NULL;
  if (list && num)
  {
    programs = malloc(num*sizeof(cl_program));
    for (int i = 0; i < num; i++)
    {
      programs[i] = list[i]->program;
    }
  }
  return programs;
}

CL_API_ENTRY cl_int CL_API_CALL
_clCompileProgram_(cl_program            program ,
                   cl_uint               num_devices ,
                   const cl_device_id *  device_list ,
                   const char *          options ,
                   cl_uint               num_input_headers ,
                   const cl_program *    input_headers,
                   const char **         header_include_names ,
                   void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
                   void *                user_data) CL_API_SUFFIX__VERSION_1_2
{
  cl_device_id *_devices = createDeviceList(num_devices, device_list);
  cl_program *_headers = createProgramList(num_input_headers, input_headers);

  // Call original function
  const char *_options = options ? options : "";
  size_t sz = strlen(_options) + strlen("-cl-kernel-arg-info") + 2;
  char *buildOptions = malloc(sz);
  cl_int err = clCompileProgram(
    program->program,
    num_devices,
    _devices,
    buildOptions,
    num_input_headers,
    _headers,
    header_include_names,
    pfn_notify,
    user_data
  );
  free(buildOptions);

  if (_devices)
  {
    free(_devices);
  }
  if (_headers)
  {
    free(_headers);
  }
  return err;
}

CL_API_ENTRY cl_program CL_API_CALL
_clLinkProgram_(cl_context            context ,
                cl_uint               num_devices ,
                const cl_device_id *  device_list ,
                const char *          options ,
                cl_uint               num_input_programs ,
                const cl_program *    input_programs ,
                void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
                void *                user_data ,
                cl_int *              errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  cl_device_id *_devices = createDeviceList(num_devices, device_list);
  cl_program *_programs = createProgramList(num_input_programs, input_programs);

  // Call original function
  cl_int err;
  cl_program _program = clLinkProgram(
    context->context,
    num_devices,
    _devices,
    options,
    num_input_programs,
    _programs,
    pfn_notify,
    user_data,
    &err
  );

  // Create wrapper object
  cl_program program = NULL;
  if (err == CL_SUCCESS)
  {
    program = malloc(sizeof(struct _cl_program));
    program->dispatch = context->dispatch;
    program->program = _program;
    program->context = context;
  }

  if (_devices)
  {
    free(_devices);
  }
  if (_programs)
  {
    free(_programs);
  }
  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return program;
}

CL_API_ENTRY cl_int CL_API_CALL
_clUnloadPlatformCompiler_(cl_platform_id  platform) CL_API_SUFFIX__VERSION_1_2
{
  return clUnloadPlatformCompiler(platform->platform);
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetProgramInfo_(cl_program          program ,
                   cl_program_info     param_name ,
                   size_t              param_value_size ,
                   void *              param_value ,
                   size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_PROGRAM_CONTEXT)
  {
    if (param_value_size && param_value_size < sizeof(cl_context))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &program->context, sizeof(cl_context));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_context);
    }
    return CL_SUCCESS;
  }
  else if (param_name == CL_PROGRAM_DEVICES)
  {
    size_t sz = program->context->numDevices * sizeof(cl_device_id);
    if (param_value_size && param_value_size < sz)
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, program->context->devices, sz);
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sz;
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetProgramInfo(
      program->program,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetProgramBuildInfo_(cl_program             program ,
                        cl_device_id           device ,
                        cl_program_build_info  param_name ,
                        size_t                 param_value_size ,
                        void *                 param_value ,
                        size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return clGetProgramBuildInfo(
    program->program,
    device->device,
    param_name,
    param_value_size,
    param_value,
    param_value_size_ret
  );
}

CL_API_ENTRY cl_kernel CL_API_CALL
_clCreateKernel_(cl_program       program ,
                 const char *     kernel_name ,
                 cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Call original function
  cl_int err;
  cl_kernel _kernel = clCreateKernel(
    program->program,
    kernel_name,
    &err
  );

  // Create wrapper object
  cl_kernel kernel = NULL;
  if (err == CL_SUCCESS)
  {
    kernel = malloc(sizeof(struct _cl_kernel));
    kernel->dispatch = program->dispatch;
    kernel->kernel = _kernel;
    kernel->program = program;
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return kernel;
}

CL_API_ENTRY cl_int CL_API_CALL
_clCreateKernelsInProgram_(cl_program      program ,
                           cl_uint         num_kernels ,
                           cl_kernel *     kernels ,
                           cl_uint *       num_kernels_ret) CL_API_SUFFIX__VERSION_1_0
{
  cl_kernel *_kernels = NULL;
  if (kernels)
  {
    _kernels = malloc(num_kernels*sizeof(cl_kernel));
  }

  // Call original function
  cl_uint num;
  cl_int err = clCreateKernelsInProgram(
    program->program,
    num_kernels,
    _kernels,
    &num
  );

  // Create wrapper objects
  if (err == CL_SUCCESS && kernels)
  {
    for (int i = 0; i < num; i++)
    {
      kernels[i] = malloc(sizeof(struct _cl_kernel));
      kernels[i]->dispatch = program->dispatch;
      kernels[i]->kernel = _kernels[i];
      kernels[i]->program = program;
    }
  }

  if (_kernels)
  {
    free(_kernels);
  }
  if (num_kernels_ret)
  {
    *num_kernels_ret = num;
  }
  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainKernel_(cl_kernel     kernel) CL_API_SUFFIX__VERSION_1_0
{
  return clRetainKernel(kernel->kernel);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseKernel_(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
  return clReleaseKernel(kernel->kernel);
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetKernelArg_(cl_kernel     kernel ,
                 cl_uint       arg_index ,
                 size_t        arg_size ,
                 const void *  arg_value) CL_API_SUFFIX__VERSION_1_0
{
  // Get argument address qualifier to determine if it's a memory object
  cl_kernel_arg_address_qualifier address;
  cl_int err = clGetKernelArgInfo(
    kernel->kernel,
    arg_index,
    CL_KERNEL_ARG_ADDRESS_QUALIFIER,
    sizeof(address),
    &address,
    NULL
  );
  if (err != CL_SUCCESS)
  {
    return err;
  }

  // Get argument type name to determine if it's a sampler
  size_t sz;
  clGetKernelArgInfo(
    kernel->kernel,
    arg_index,
    CL_KERNEL_ARG_TYPE_NAME,
    0,
    NULL,
    &sz
  );
  char *type = malloc(++sz);
  clGetKernelArgInfo(
    kernel->kernel,
    arg_index,
    CL_KERNEL_ARG_TYPE_NAME,
    sz,
    type,
    NULL
  );

  // If global/constant memory or sampler, get real object
  const void *value = arg_value;
  if (strcmp(type, "sampler_t") == 0)
  {
    value = &(*(cl_sampler*)arg_value)->sampler;
  }
  else if (arg_value && (
      address == CL_KERNEL_ARG_ADDRESS_GLOBAL ||
      address == CL_KERNEL_ARG_ADDRESS_CONSTANT))
  {
    cl_mem buffer = *(cl_mem*)arg_value;
    if (buffer)
    {
      value = &(buffer->mem);
    }
    else
    {
      value = NULL;
    }
  }

  // Call original function
  err = clSetKernelArg(
    kernel->kernel,
    arg_index,
    arg_size,
    value
  );

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetKernelInfo_(cl_kernel        kernel ,
                  cl_kernel_info   param_name ,
                  size_t           param_value_size ,
                  void *           param_value ,
                  size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_KERNEL_CONTEXT)
  {
    if (param_value_size && param_value_size < sizeof(cl_context))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &(kernel->program->context), sizeof(cl_context));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_context);
    }
    return CL_SUCCESS;
  }
  else if (param_name == CL_KERNEL_PROGRAM)
  {
    if (param_value_size && param_value_size < sizeof(cl_program))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &kernel->program, sizeof(cl_program));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_program);
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetKernelInfo(
      kernel->kernel,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetKernelArgInfo_(cl_kernel        kernel ,
                     cl_uint          arg_indx ,
                     cl_kernel_arg_info   param_name ,
                     size_t           param_value_size ,
                     void *           param_value ,
                     size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_2
{
  return clGetKernelArgInfo(
    kernel->kernel,
    arg_indx,
    param_name,
    param_value_size,
    param_value,
    param_value_size_ret
  );
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetKernelWorkGroupInfo_(cl_kernel                   kernel ,
                           cl_device_id                device ,
                           cl_kernel_work_group_info   param_name ,
                           size_t                      param_value_size ,
                           void *                      param_value ,
                           size_t *                    param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return clGetKernelWorkGroupInfo(
    kernel->kernel,
    device->device,
    param_name,
    param_value_size,
    param_value,
    param_value_size_ret
  );
}

// Utility function to convert event list into real event list
cl_event* createEventList(cl_uint num, const cl_event *list)
{
  cl_event *result = NULL;
  if (num > 0 && list)
  {
    result = malloc(num*sizeof(cl_event));
    for (int i = 0; i < num; i++)
    {
      result[i] = list[i]->event;
    }
  }
  return result;
}

CL_API_ENTRY cl_int CL_API_CALL
_clWaitForEvents_(cl_uint              num_events ,
                  const cl_event *     event_list) CL_API_SUFFIX__VERSION_1_0
{
  if (num_events == 0)
  {
    return CL_SUCCESS;
  }

  // Call original function
  cl_event *_events = createEventList(num_events, event_list);
  cl_int err = clWaitForEvents(num_events, _events);
  free(_events);
  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetEventInfo_(cl_event          event ,
                 cl_event_info     param_name ,
                 size_t            param_value_size ,
                 void *            param_value ,
                 size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  if (param_name == CL_EVENT_CONTEXT)
  {
    if (param_value_size && param_value_size < sizeof(cl_context))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &(event->context), sizeof(cl_context));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_context);
    }
    return CL_SUCCESS;
  }
  else if (param_name == CL_EVENT_COMMAND_QUEUE)
  {
    if (param_value_size && param_value_size < sizeof(cl_command_queue))
    {
      return CL_INVALID_VALUE;
    }
    if (param_value)
    {
      memcpy(param_value, &event->queue, sizeof(cl_command_queue));
    }
    if (param_value_size_ret)
    {
      *param_value_size_ret = sizeof(cl_command_queue);
    }
    return CL_SUCCESS;
  }
  else
  {
    return clGetEventInfo(
      event->event,
      param_name,
      param_value_size,
      param_value,
      param_value_size_ret
    );
  }
}

CL_API_ENTRY cl_event CL_API_CALL
_clCreateUserEvent_(cl_context     context ,
                    cl_int *       errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
  // Call original function
  cl_int err;
  cl_event _event = clCreateUserEvent(
    context->context,
    &err
  );

  // Create wrapper object
  cl_event event = NULL;
  if (err == CL_SUCCESS)
  {
    event = malloc(sizeof(struct _cl_event));
    event->dispatch = context->dispatch;
    event->event = _event;
    event->context = context;
    event->queue = NULL;
  }

  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return event;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainEvent_(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  return clRetainEvent(event->event);
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseEvent_(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  return clReleaseEvent(event->event);
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetUserEventStatus_(cl_event    event ,
                       cl_int      execution_status) CL_API_SUFFIX__VERSION_1_1
{
  return clSetUserEventStatus(event->event, execution_status);
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetEventCallback_(cl_event     event ,
                     cl_int       command_exec_callback_type ,
                     void (CL_CALLBACK *  pfn_notify)(cl_event, cl_int, void *),
                     void *       user_data) CL_API_SUFFIX__VERSION_1_1
{
  return clSetEventCallback(
    event->event,
    command_exec_callback_type,
    pfn_notify,
    user_data
  );
}

/* Profiling APIs  */
CL_API_ENTRY cl_int CL_API_CALL
_clGetEventProfilingInfo_(cl_event             event ,
                          cl_profiling_info    param_name ,
                          size_t               param_value_size ,
                          void *               param_value ,
                          size_t *             param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return clGetEventProfilingInfo(
    event->event,
    param_name,
    param_value_size,
    param_value,
    param_value_size_ret
  );
}

CL_API_ENTRY cl_int CL_API_CALL
_clFlush_(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return clFlush(command_queue->queue);
}

CL_API_ENTRY cl_int CL_API_CALL
_clFinish_(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return clFinish(command_queue->queue);
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueReadBuffer_(cl_command_queue     command_queue ,
                      cl_mem               buffer ,
                      cl_bool              blocking_read ,
                      size_t               offset ,
                      size_t               cb ,
                      void *               ptr ,
                      cl_uint              num_events_in_wait_list ,
                      const cl_event *     event_wait_list ,
                      cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueReadBuffer(
    command_queue->queue,
    buffer->mem,
    blocking_read,
    offset,
    cb,
    ptr,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueReadBufferRect_(cl_command_queue     command_queue ,
                          cl_mem               buffer ,
                          cl_bool              blocking_read ,
                          const size_t *       buffer_origin ,
                          const size_t *       host_origin ,
                          const size_t *       region ,
                          size_t               buffer_row_pitch ,
                          size_t               buffer_slice_pitch ,
                          size_t               host_row_pitch ,
                          size_t               host_slice_pitch ,
                          void *               ptr ,
                          cl_uint              num_events_in_wait_list ,
                          const cl_event *     event_wait_list ,
                          cl_event *           event) CL_API_SUFFIX__VERSION_1_1
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueReadBufferRect(
    command_queue->queue,
    buffer->mem,
    blocking_read,
    buffer_origin,
    host_origin,
    region,
    buffer_row_pitch,
    buffer_slice_pitch,
    host_row_pitch,
    host_slice_pitch,
    ptr,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueWriteBuffer_(cl_command_queue    command_queue ,
                       cl_mem              buffer ,
                       cl_bool             blocking_write ,
                       size_t              offset ,
                       size_t              cb ,
                       const void *        ptr ,
                       cl_uint             num_events_in_wait_list ,
                       const cl_event *    event_wait_list ,
                       cl_event *          event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueWriteBuffer(
    command_queue->queue,
    buffer->mem,
    blocking_write,
    offset,
    cb,
    ptr,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueWriteBufferRect_(cl_command_queue     command_queue ,
                           cl_mem               buffer ,
                           cl_bool              blocking_write ,
                           const size_t *       buffer_origin ,
                           const size_t *       host_origin ,
                           const size_t *       region ,
                           size_t               buffer_row_pitch ,
                           size_t               buffer_slice_pitch ,
                           size_t               host_row_pitch ,
                           size_t               host_slice_pitch ,
                           const void *         ptr ,
                           cl_uint              num_events_in_wait_list ,
                           const cl_event *     event_wait_list ,
                           cl_event *           event) CL_API_SUFFIX__VERSION_1_1
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueWriteBufferRect(
    command_queue->queue,
    buffer->mem,
    blocking_write,
    buffer_origin,
    host_origin,
    region,
    buffer_row_pitch,
    buffer_slice_pitch,
    host_row_pitch,
    host_slice_pitch,
    ptr,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueCopyBuffer_(cl_command_queue     command_queue ,
                      cl_mem               src_buffer ,
                      cl_mem               dst_buffer ,
                      size_t               src_offset ,
                      size_t               dst_offset ,
                      size_t               cb ,
                      cl_uint              num_events_in_wait_list ,
                      const cl_event *     event_wait_list ,
                      cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueCopyBuffer(
    command_queue->queue,
    src_buffer->mem,
    dst_buffer->mem,
    src_offset,
    dst_offset,
    cb,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueCopyBufferRect_(cl_command_queue     command_queue ,
                          cl_mem               src_buffer ,
                          cl_mem               dst_buffer ,
                          const size_t *       src_origin ,
                          const size_t *       dst_origin ,
                          const size_t *       region ,
                          size_t               src_row_pitch ,
                          size_t               src_slice_pitch ,
                          size_t               dst_row_pitch ,
                          size_t               dst_slice_pitch ,
                          cl_uint              num_events_in_wait_list ,
                          const cl_event *     event_wait_list ,
                          cl_event *           event) CL_API_SUFFIX__VERSION_1_1
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueCopyBufferRect(
    command_queue->queue,
    src_buffer->mem,
    dst_buffer->mem,
    src_origin,
    dst_origin,
    region,
    src_row_pitch,
    src_slice_pitch,
    dst_row_pitch,
    dst_slice_pitch,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueFillBuffer_(cl_command_queue    command_queue ,
                      cl_mem              buffer ,
                      const void *        pattern ,
                      size_t              pattern_size ,
                      size_t              offset ,
                      size_t              cb ,
                      cl_uint             num_events_in_wait_list ,
                      const cl_event *    event_wait_list ,
                      cl_event *          event) CL_API_SUFFIX__VERSION_1_2
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueFillBuffer(
    command_queue->queue,
    buffer->mem,
    pattern,
    pattern_size,
    offset,
    cb,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueFillImage_(cl_command_queue    command_queue ,
                     cl_mem              image ,
                     const void *        fill_color ,
                     const size_t *      origin ,
                     const size_t *      region ,
                     cl_uint             num_events_in_wait_list ,
                     const cl_event *    event_wait_list ,
                     cl_event *          event) CL_API_SUFFIX__VERSION_1_2
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueFillImage(
    command_queue->queue,
    image->mem,
    fill_color,
    origin,
    region,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueReadImage_(cl_command_queue      command_queue ,
                     cl_mem                image ,
                     cl_bool               blocking_read ,
                     const size_t *        origin ,
                     const size_t *        region ,
                     size_t                row_pitch ,
                     size_t                slice_pitch ,
                     void *                ptr ,
                     cl_uint               num_events_in_wait_list ,
                     const cl_event *      event_wait_list ,
                     cl_event *            event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueReadImage(
    command_queue->queue,
    image->mem,
    blocking_read,
    origin,
    region,
    row_pitch,
    slice_pitch,
    ptr,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueWriteImage_(cl_command_queue     command_queue ,
                      cl_mem               image ,
                      cl_bool              blocking_write ,
                      const size_t *       origin ,
                      const size_t *       region ,
                      size_t               input_row_pitch ,
                      size_t               input_slice_pitch ,
                      const void *         ptr ,
                      cl_uint              num_events_in_wait_list ,
                      const cl_event *     event_wait_list ,
                      cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueWriteImage(
    command_queue->queue,
    image->mem,
    blocking_write,
    origin,
    region,
    input_row_pitch,
    input_slice_pitch,
    ptr,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueCopyImage_(cl_command_queue      command_queue ,
                     cl_mem                src_image ,
                     cl_mem                dst_image ,
                     const size_t *        src_origin ,
                     const size_t *        dst_origin ,
                     const size_t *        region ,
                     cl_uint               num_events_in_wait_list ,
                     const cl_event *      event_wait_list ,
                     cl_event *            event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueCopyImage(
    command_queue->queue,
    src_image->mem,
    dst_image->mem,
    src_origin,
    dst_origin,
    region,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueCopyImageToBuffer_(cl_command_queue  command_queue ,
                             cl_mem            src_image ,
                             cl_mem            dst_buffer ,
                             const size_t *    src_origin ,
                             const size_t *    region ,
                             size_t            dst_offset ,
                             cl_uint           num_events_in_wait_list ,
                             const cl_event *  event_wait_list ,
                             cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueCopyImageToBuffer(
    command_queue->queue,
    src_image->mem,
    dst_buffer->mem,
    src_origin,
    region,
    dst_offset,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueCopyBufferToImage_(cl_command_queue  command_queue ,
                             cl_mem            src_buffer ,
                             cl_mem            dst_image ,
                             size_t            src_offset ,
                             const size_t *    dst_origin ,
                             const size_t *    region ,
                             cl_uint           num_events_in_wait_list ,
                             const cl_event *  event_wait_list ,
                             cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueCopyBufferToImage(
    command_queue->queue,
    src_buffer->mem,
    dst_image->mem,
    src_offset,
    dst_origin,
    region,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY void * CL_API_CALL
_clEnqueueMapBuffer_(cl_command_queue  command_queue ,
                     cl_mem            buffer ,
                     cl_bool           blocking_map ,
                     cl_map_flags      map_flags ,
                     size_t            offset ,
                     size_t            cb ,
                     cl_uint           num_events_in_wait_list ,
                     const cl_event *  event_wait_list ,
                     cl_event *        event ,
                     cl_int *          errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err;
  void *ret = clEnqueueMapBuffer(
    command_queue->queue,
    buffer->mem,
    blocking_map,
    map_flags,
    offset,
    cb,
    num_events_in_wait_list,
    _wait_list,
    _event,
    &err
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }
  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return ret;
}

CL_API_ENTRY void * CL_API_CALL
_clEnqueueMapImage_(cl_command_queue   command_queue ,
                    cl_mem             image ,
                    cl_bool            blocking_map ,
                    cl_map_flags       map_flags ,
                    const size_t *     origin ,
                    const size_t *     region ,
                    size_t *           image_row_pitch ,
                    size_t *           image_slice_pitch ,
                    cl_uint            num_events_in_wait_list ,
                    const cl_event *   event_wait_list ,
                    cl_event *         event ,
                    cl_int *           errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err;
  void *ret = clEnqueueMapImage(
    command_queue->queue,
    image->mem,
    blocking_map,
    map_flags,
    origin,
    region,
    image_row_pitch,
    image_slice_pitch,
    num_events_in_wait_list,
    _wait_list,
    _event,
    &err
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }
  if (errcode_ret)
  {
    *errcode_ret = err;
  }
  return ret;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueUnmapMemObject_(cl_command_queue  command_queue ,
                          cl_mem            memobj ,
                          void *            mapped_ptr ,
                          cl_uint           num_events_in_wait_list ,
                          const cl_event *   event_wait_list ,
                          cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueUnmapMemObject(
    command_queue->queue,
    memobj->mem,
    mapped_ptr,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueMigrateMemObjects_(cl_command_queue        command_queue ,
                             cl_uint                 num_mem_objects ,
                             const cl_mem *          mem_objects ,
                             cl_mem_migration_flags  flags ,
                             cl_uint                 num_events_in_wait_list ,
                             const cl_event *        event_wait_list ,
                             cl_event *              event) CL_API_SUFFIX__VERSION_1_2
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Build real mem object list
  cl_mem *_objects = NULL;
  if (num_mem_objects > 0 && mem_objects)
  {
    _objects = malloc(num_mem_objects*sizeof(cl_mem));
    for (int i = 0; i < num_mem_objects; i++)
    {
      _objects[i] = mem_objects[i]->mem;
    }
  }

  // Call original function
  cl_int err = clEnqueueMigrateMemObjects(
    command_queue->queue,
    num_mem_objects,
    _objects,
    flags,
    num_events_in_wait_list,
    _wait_list,
    _event
  );
  if (_objects)
  {
    free(_objects);
  }

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueNDRangeKernel_(cl_command_queue  command_queue ,
                         cl_kernel         kernel ,
                         cl_uint           work_dim ,
                         const size_t *    global_work_offset ,
                         const size_t *    global_work_size ,
                         const size_t *    local_work_size ,
                         cl_uint           num_events_in_wait_list ,
                         const cl_event *  event_wait_list ,
                         cl_event *        event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueNDRangeKernel(
    command_queue->queue,
    kernel->kernel,
    work_dim,
    global_work_offset,
    global_work_size,
    local_work_size,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueTask_(cl_command_queue   command_queue ,
                cl_kernel          kernel ,
                cl_uint            num_events_in_wait_list ,
                const cl_event *   event_wait_list ,
                cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueTask(
    command_queue->queue,
    kernel->kernel,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueNativeKernel_(cl_command_queue   command_queue ,
                        void (CL_CALLBACK *user_func)(void *),
                        void *             args ,
                        size_t             cb_args ,
                        cl_uint            num_mem_objects ,
                        const cl_mem *     mem_list ,
                        const void **      args_mem_loc ,
                        cl_uint            num_events_in_wait_list ,
                        const cl_event *   event_wait_list ,
                        cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY void * CL_API_CALL
_clGetExtensionFunctionAddressForPlatform_(cl_platform_id  platform ,
                                           const char *    func_name) CL_API_SUFFIX__VERSION_1_2
{
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueMarkerWithWaitList_(cl_command_queue  command_queue ,
                              cl_uint            num_events_in_wait_list ,
                              const cl_event *   event_wait_list ,
                              cl_event *         event) CL_API_SUFFIX__VERSION_1_2

{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueMarkerWithWaitList(
    command_queue->queue,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

extern CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueBarrierWithWaitList_(cl_command_queue  command_queue ,
                               cl_uint            num_events_in_wait_list ,
                               const cl_event *   event_wait_list ,
                               cl_event *         event) CL_API_SUFFIX__VERSION_1_2
{
  // Initialize event arguments
  cl_event *_wait_list = createEventList(
    num_events_in_wait_list,
    event_wait_list
  );
  cl_event *_event = NULL;
  if (event)
  {
    _event = malloc(sizeof(cl_event));
  }

  // Call original function
  cl_int err = clEnqueueBarrierWithWaitList(
    command_queue->queue,
    num_events_in_wait_list,
    _wait_list,
    _event
  );

  // Create wrapper object
  if (err == CL_SUCCESS && event)
  {
    *event = malloc(sizeof(struct _cl_event));
    (*event)->dispatch = command_queue->dispatch;
    (*event)->event = *_event;
    (*event)->context = command_queue->context;
    (*event)->queue = command_queue;
    free(_event);
  }
  if (_wait_list)
  {
    free(_wait_list);
  }

  return err;
}

extern CL_API_ENTRY cl_int CL_API_CALL
_clSetPrintfCallback_(cl_context           context ,
                      void (CL_CALLBACK *  pfn_notify)(cl_context  program ,
                                                       cl_uint printf_data_len ,
                                                       char *  printf_data_ptr ,
                                                       void *  user_data),
                      void *               user_data) CL_API_SUFFIX__VERSION_1_2
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueMarker_(cl_command_queue     command_queue ,
                  cl_event *           event) CL_API_SUFFIX__VERSION_1_0
{
  return _clEnqueueMarkerWithWaitList_(
    command_queue,
    0,
    NULL,
    event
  );
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueWaitForEvents_(cl_command_queue  command_queue ,
                         cl_uint           num_events ,
                         const cl_event *  event_list) CL_API_SUFFIX__VERSION_1_0
{
  cl_event *_events = createEventList(num_events, event_list);
  cl_int err = clEnqueueWaitForEvents(
    command_queue,
    num_events,
    _events
  );
  free(_events);
  return err;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueBarrier_(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return clEnqueueBarrier(command_queue->queue);
}

CL_API_ENTRY void* CL_API_CALL
_clGetExtensionFunctionAddress_(const char *funcname) CL_API_SUFFIX__VERSION_1_2
{
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateFromGLBuffer_(cl_context      context ,
                       cl_mem_flags    flags ,
                       cl_GLuint       bufret_mem ,
                       int *           errcode_ret ) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateFromGLTexture_(cl_context       context ,
                        cl_mem_flags     flags ,
                        cl_GLenum        target ,
                        cl_GLint         miplevel ,
                        cl_GLuint        texture ,
                        cl_int *         errcode_ret ) CL_API_SUFFIX__VERSION_1_2
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateFromGLTexture2D_(cl_context       context,
                          cl_mem_flags     flags,
                          cl_GLenum        target,
                          cl_GLint         miplevel,
                          cl_GLuint        texture,
                          cl_int *         errcode_ret ) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateFromGLTexture3D_(cl_context       context,
                          cl_mem_flags     flags,
                          cl_GLenum        target,
                          cl_GLint         miplevel,
                          cl_GLuint        texture,
                          cl_int *         errcode_ret ) CL_API_SUFFIX__VERSION_1_0

{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateFromGLRenderbuffer_(cl_context    context,
                             cl_mem_flags  flags,
                             cl_GLuint     renderbuffer,
                             cl_int *      errcode_ret ) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetGLObjectInfo_(cl_mem                 memobj,
                    cl_gl_object_type *    gl_object_type,
                    cl_GLuint *            gl_object_name ) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetGLTextureInfo_(cl_mem                memobj,
                     cl_gl_texture_info    param_name,
                     size_t                param_value_size,
                     void *                param_value,
                     size_t *              param_value_size_ret ) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueAcquireGLObjects_(cl_command_queue       command_queue,
                            cl_uint                num_objects,
                            const cl_mem *         mem_objects,
                            cl_uint                num_events_in_wait_list,
                            const cl_event *       event_wait_list,
                            cl_event *             event ) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueReleaseGLObjects_(cl_command_queue       command_queue,
                            cl_uint                num_objects,
                            const cl_mem *         mem_objects,
                            cl_uint                num_events_in_wait_list,
                            const cl_event *       event_wait_list,
                            cl_event *             event ) CL_API_SUFFIX__VERSION_1_0

{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetGLContextInfoKHR_(const cl_context_properties *  properties,
                        cl_gl_context_info             param_name,
                        size_t                         param_value_size,
                        void *                         param_value,
                        size_t *                       param_value_size_ret ) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_event CL_API_CALL
_clCreateEventFromGLsyncKHR_(cl_context            context ,
                             cl_GLsync             cl_GLsync ,
                             cl_int *              errcode_ret ) //CL_EXT_SUFFIX__VERSION_1_1

{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

KHRicdVendorDispatch* createDispatchTable()
{
  // Allocate table
  KHRicdVendorDispatch *table = malloc(sizeof(KHRicdVendorDispatch));
  if (!table)
  {
    return NULL;
  }
  memset(table, 0, sizeof(KHRicdVendorDispatch));

#define DISPATCH_TABLE_ENTRY(fn) table->fn = _##fn##_;

  // OpenCL 1.0
  //DISPATCH_TABLE_ENTRY(clGetPlatformIDs);
  DISPATCH_TABLE_ENTRY(clGetPlatformInfo);
  DISPATCH_TABLE_ENTRY(clGetDeviceIDs);
  DISPATCH_TABLE_ENTRY(clGetDeviceInfo);
  DISPATCH_TABLE_ENTRY(clCreateContext);
  DISPATCH_TABLE_ENTRY(clCreateContextFromType);
  DISPATCH_TABLE_ENTRY(clRetainContext);
  DISPATCH_TABLE_ENTRY(clReleaseContext);
  DISPATCH_TABLE_ENTRY(clGetContextInfo);
  DISPATCH_TABLE_ENTRY(clCreateCommandQueue);
  DISPATCH_TABLE_ENTRY(clRetainCommandQueue);
  DISPATCH_TABLE_ENTRY(clReleaseCommandQueue);
  DISPATCH_TABLE_ENTRY(clGetCommandQueueInfo);
  DISPATCH_TABLE_ENTRY(clSetCommandQueueProperty);
  DISPATCH_TABLE_ENTRY(clCreateBuffer);
  DISPATCH_TABLE_ENTRY(clCreateImage2D);
  DISPATCH_TABLE_ENTRY(clCreateImage3D);
  DISPATCH_TABLE_ENTRY(clRetainMemObject);
  DISPATCH_TABLE_ENTRY(clReleaseMemObject);
  DISPATCH_TABLE_ENTRY(clGetSupportedImageFormats);
  DISPATCH_TABLE_ENTRY(clGetMemObjectInfo);
  DISPATCH_TABLE_ENTRY(clGetImageInfo);
  DISPATCH_TABLE_ENTRY(clCreateSampler);
  DISPATCH_TABLE_ENTRY(clRetainSampler);
  DISPATCH_TABLE_ENTRY(clReleaseSampler);
  DISPATCH_TABLE_ENTRY(clGetSamplerInfo);
  DISPATCH_TABLE_ENTRY(clCreateProgramWithSource);
  DISPATCH_TABLE_ENTRY(clCreateProgramWithBinary);
  DISPATCH_TABLE_ENTRY(clRetainProgram);
  DISPATCH_TABLE_ENTRY(clReleaseProgram);
  DISPATCH_TABLE_ENTRY(clBuildProgram);
  DISPATCH_TABLE_ENTRY(clUnloadCompiler);
  DISPATCH_TABLE_ENTRY(clGetProgramInfo);
  DISPATCH_TABLE_ENTRY(clGetProgramBuildInfo);
  DISPATCH_TABLE_ENTRY(clCreateKernel);
  DISPATCH_TABLE_ENTRY(clCreateKernelsInProgram);
  DISPATCH_TABLE_ENTRY(clRetainKernel);
  DISPATCH_TABLE_ENTRY(clReleaseKernel);
  DISPATCH_TABLE_ENTRY(clSetKernelArg);
  DISPATCH_TABLE_ENTRY(clGetKernelInfo);
  DISPATCH_TABLE_ENTRY(clGetKernelWorkGroupInfo);
  DISPATCH_TABLE_ENTRY(clWaitForEvents);
  DISPATCH_TABLE_ENTRY(clGetEventInfo);
  DISPATCH_TABLE_ENTRY(clRetainEvent);
  DISPATCH_TABLE_ENTRY(clReleaseEvent);
  DISPATCH_TABLE_ENTRY(clGetEventProfilingInfo);
  DISPATCH_TABLE_ENTRY(clFlush);
  DISPATCH_TABLE_ENTRY(clFinish);
  DISPATCH_TABLE_ENTRY(clEnqueueReadBuffer);
  DISPATCH_TABLE_ENTRY(clEnqueueWriteBuffer);
  DISPATCH_TABLE_ENTRY(clEnqueueCopyBuffer);
  DISPATCH_TABLE_ENTRY(clEnqueueReadImage);
  DISPATCH_TABLE_ENTRY(clEnqueueWriteImage);
  DISPATCH_TABLE_ENTRY(clEnqueueCopyImage);
  DISPATCH_TABLE_ENTRY(clEnqueueCopyImageToBuffer);
  DISPATCH_TABLE_ENTRY(clEnqueueCopyBufferToImage);
  DISPATCH_TABLE_ENTRY(clEnqueueMapBuffer);
  DISPATCH_TABLE_ENTRY(clEnqueueMapImage);
  DISPATCH_TABLE_ENTRY(clEnqueueUnmapMemObject);
  DISPATCH_TABLE_ENTRY(clEnqueueNDRangeKernel);
  DISPATCH_TABLE_ENTRY(clEnqueueTask);
  DISPATCH_TABLE_ENTRY(clEnqueueNativeKernel);
  DISPATCH_TABLE_ENTRY(clEnqueueMarker);
  DISPATCH_TABLE_ENTRY(clEnqueueWaitForEvents);
  DISPATCH_TABLE_ENTRY(clEnqueueBarrier);
  DISPATCH_TABLE_ENTRY(clGetExtensionFunctionAddress);
  DISPATCH_TABLE_ENTRY(clCreateFromGLBuffer);
  DISPATCH_TABLE_ENTRY(clCreateFromGLTexture2D);
  DISPATCH_TABLE_ENTRY(clCreateFromGLTexture3D);
  DISPATCH_TABLE_ENTRY(clCreateFromGLRenderbuffer);
  DISPATCH_TABLE_ENTRY(clGetGLObjectInfo);
  DISPATCH_TABLE_ENTRY(clGetGLTextureInfo);
  DISPATCH_TABLE_ENTRY(clEnqueueAcquireGLObjects);
  DISPATCH_TABLE_ENTRY(clEnqueueReleaseGLObjects);

  // cl_khr_gl_sharing
  DISPATCH_TABLE_ENTRY(clGetGLContextInfoKHR);

  // cl_khr_d3d10_sharing (windows-only)
#if defined(_WIN32)
  //DISPATCH_TABLE_ENTRY(clGetDeviceIDsFromD3D10KHR);
  //DISPATCH_TABLE_ENTRY(clCreateFromD3D10BufferKHR);
  //DISPATCH_TABLE_ENTRY(clCreateFromD3D10Texture2DKHR);
  //DISPATCH_TABLE_ENTRY(clCreateFromD3D10Texture3DKHR);
  //DISPATCH_TABLE_ENTRY(clEnqueueAcquireD3D10ObjectsKHR);
  //DISPATCH_TABLE_ENTRY(clEnqueueReleaseD3D10ObjectsKHR);
#endif

  // OpenCL 1.1
  DISPATCH_TABLE_ENTRY(clSetEventCallback);
  DISPATCH_TABLE_ENTRY(clCreateSubBuffer);
  DISPATCH_TABLE_ENTRY(clSetMemObjectDestructorCallback);
  DISPATCH_TABLE_ENTRY(clCreateUserEvent);
  DISPATCH_TABLE_ENTRY(clSetUserEventStatus);
  DISPATCH_TABLE_ENTRY(clEnqueueReadBufferRect);
  DISPATCH_TABLE_ENTRY(clEnqueueWriteBufferRect);
  DISPATCH_TABLE_ENTRY(clEnqueueCopyBufferRect);

  //DISPATCH_TABLE_ENTRY(clCreateSubDevicesEXT);
  //DISPATCH_TABLE_ENTRY(clRetainDeviceEXT);
  //DISPATCH_TABLE_ENTRY(clReleaseDeviceEXT);

  DISPATCH_TABLE_ENTRY(clCreateEventFromGLsyncKHR);

  DISPATCH_TABLE_ENTRY(clCreateSubDevices);
  DISPATCH_TABLE_ENTRY(clRetainDevice);
  DISPATCH_TABLE_ENTRY(clReleaseDevice);
  DISPATCH_TABLE_ENTRY(clCreateImage);
  DISPATCH_TABLE_ENTRY(clCreateProgramWithBuiltInKernels);
  DISPATCH_TABLE_ENTRY(clCompileProgram);
  DISPATCH_TABLE_ENTRY(clLinkProgram);
  DISPATCH_TABLE_ENTRY(clUnloadPlatformCompiler);
  DISPATCH_TABLE_ENTRY(clGetKernelArgInfo);
  DISPATCH_TABLE_ENTRY(clEnqueueFillBuffer);
  DISPATCH_TABLE_ENTRY(clEnqueueFillImage);
  DISPATCH_TABLE_ENTRY(clEnqueueMigrateMemObjects);
  DISPATCH_TABLE_ENTRY(clEnqueueMarkerWithWaitList);
  DISPATCH_TABLE_ENTRY(clEnqueueBarrierWithWaitList);
  DISPATCH_TABLE_ENTRY(clGetExtensionFunctionAddressForPlatform);
  DISPATCH_TABLE_ENTRY(clCreateFromGLTexture);

  return table;
}
