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

  cl_int err = clGetDeviceIDs(
    platform->platform,
    device_type,
    num_entries,
    _devices,
    num_devices
  );

  if (devices && err == CL_SUCCESS)
  {
    // Create wrapper object for each real device
    for (int i = 0; i < *num_devices && i < num_entries; i++)
    {
      devices[i] = malloc(sizeof(struct _cl_device_id));
      devices[i]->dispatch = platform->dispatch;
      devices[i]->platform = platform;
      devices[i]->device = _devices[i];
    }
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
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseDevice_(cl_device_id device) CL_API_SUFFIX__VERSION_1_2
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_context CL_API_CALL
_clCreateContext_(const cl_context_properties * properties,
                  cl_uint                       num_devices ,
                  const cl_device_id *          devices,
                  void (CL_CALLBACK * pfn_notify)(const char *, const void *, size_t, void *),
                  void *                        user_data,
                  cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_context CL_API_CALL
_clCreateContextFromType_(const cl_context_properties * properties,
                          cl_device_type                device_type,
                          void (CL_CALLBACK *     pfn_notify)(const char *, const void *, size_t, void *),
                          void *                        user_data,
                          cl_int *                      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainContext_(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseContext_(cl_context context) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetContextInfo_(cl_context         context,
                   cl_context_info    param_name,
                   size_t             param_value_size,
                   void *             param_value,
                   size_t *           param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_command_queue CL_API_CALL
_clCreateCommandQueue_(cl_context                     context,
                       cl_device_id                   device,
                       cl_command_queue_properties    properties,
                       cl_int *                       errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
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
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseCommandQueue_(cl_command_queue command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetCommandQueueInfo_(cl_command_queue       command_queue ,
                        cl_command_queue_info  param_name ,
                        size_t                 param_value_size ,
                        void *                 param_value ,
                        size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateBuffer_(cl_context    context ,
                 cl_mem_flags  flags ,
                 size_t        size ,
                 void *        host_ptr ,
                 cl_int *      errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateSubBuffer_(cl_mem                    buffer ,
                    cl_mem_flags              flags ,
                    cl_buffer_create_type     buffer_create_type ,
                    const void *              buffer_create_info ,
                    cl_int *                  errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_mem CL_API_CALL
_clCreateImage_(cl_context              context,
                cl_mem_flags            flags,
                const cl_image_format * image_format,
                const cl_image_desc *   image_desc,
                void *                  host_ptr,
                cl_int *                errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
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
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseMemObject_(cl_mem memobj) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetSupportedImageFormats_(cl_context           context,
                             cl_mem_flags         flags,
                             cl_mem_object_type   image_type ,
                             cl_uint              num_entries ,
                             cl_image_format *    image_formats ,
                             cl_uint *            num_image_formats) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetMemObjectInfo_(cl_mem            memobj ,
                     cl_mem_info       param_name ,
                     size_t            param_value_size ,
                     void *            param_value ,
                     size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetImageInfo_(cl_mem            image ,
                 cl_image_info     param_name ,
                 size_t            param_value_size ,
                 void *            param_value ,
                 size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetMemObjectDestructorCallback_(cl_mem  memobj ,
                                   void (CL_CALLBACK * pfn_notify)(cl_mem  memobj , void* user_data),
                                   void * user_data)             CL_API_SUFFIX__VERSION_1_1
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_sampler CL_API_CALL
_clCreateSampler_(cl_context           context ,
                  cl_bool              normalized_coords ,
                  cl_addressing_mode   addressing_mode ,
                  cl_filter_mode       filter_mode ,
                  cl_int *             errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainSampler_(cl_sampler  sampler) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseSampler_(cl_sampler  sampler) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetSamplerInfo_(cl_sampler          sampler ,
                   cl_sampler_info     param_name ,
                   size_t              param_value_size ,
                   void *              param_value ,
                   size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_program CL_API_CALL
_clCreateProgramWithSource_(cl_context         context ,
                            cl_uint            count ,
                            const char **      strings ,
                            const size_t *     lengths ,
                            cl_int *           errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
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
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_program CL_API_CALL
_clCreateProgramWithBuiltInKernels_(cl_context             context ,
                                    cl_uint                num_devices ,
                                    const cl_device_id *   device_list ,
                                    const char *           kernel_names ,
                                    cl_int *               errcode_ret) CL_API_SUFFIX__VERSION_1_2
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainProgram_(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseProgram_(cl_program  program) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clBuildProgram_(cl_program            program ,
                 cl_uint               num_devices ,
                 const cl_device_id *  device_list ,
                 const char *          options ,
                 void (CL_CALLBACK *   pfn_notify)(cl_program  program , void *  user_data),
                 void *                user_data) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clUnloadCompiler_(void) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clUnloadPlatformCompiler_(cl_platform_id  platform) CL_API_SUFFIX__VERSION_1_2
{
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetProgramInfo_(cl_program          program ,
                   cl_program_info     param_name ,
                   size_t              param_value_size ,
                   void *              param_value ,
                   size_t *            param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetProgramBuildInfo_(cl_program             program ,
                        cl_device_id           device ,
                        cl_program_build_info  param_name ,
                        size_t                 param_value_size ,
                        void *                 param_value ,
                        size_t *               param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_kernel CL_API_CALL
_clCreateKernel_(cl_program       program ,
                 const char *     kernel_name ,
                 cl_int *         errcode_ret) CL_API_SUFFIX__VERSION_1_0
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clCreateKernelsInProgram_(cl_program      program ,
                           cl_uint         num_kernels ,
                           cl_kernel *     kernels ,
                           cl_uint *       num_kernels_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainKernel_(cl_kernel     kernel) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseKernel_(cl_kernel    kernel) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetKernelArg_(cl_kernel     kernel ,
                 cl_uint       arg_index ,
                 size_t        arg_size ,
                 const void *  arg_value) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetKernelInfo_(cl_kernel        kernel ,
                  cl_kernel_info   param_name ,
                  size_t           param_value_size ,
                  void *           param_value ,
                  size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetKernelArgInfo_(cl_kernel        kernel ,
                     cl_uint          arg_indx ,
                     cl_kernel_arg_info   param_name ,
                     size_t           param_value_size ,
                     void *           param_value ,
                     size_t *         param_value_size_ret) CL_API_SUFFIX__VERSION_1_2
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetKernelWorkGroupInfo_(cl_kernel                   kernel ,
                           cl_device_id                device ,
                           cl_kernel_work_group_info   param_name ,
                           size_t                      param_value_size ,
                           void *                      param_value ,
                           size_t *                    param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clWaitForEvents_(cl_uint              num_events ,
                  const cl_event *     event_list) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clGetEventInfo_(cl_event          event ,
                 cl_event_info     param_name ,
                 size_t            param_value_size ,
                 void *            param_value ,
                 size_t *          param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_event CL_API_CALL
_clCreateUserEvent_(cl_context     context ,
                    cl_int *       errcode_ret) CL_API_SUFFIX__VERSION_1_1
{
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clRetainEvent_(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clReleaseEvent_(cl_event  event) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetUserEventStatus_(cl_event    event ,
                       cl_int      execution_status) CL_API_SUFFIX__VERSION_1_1
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clSetEventCallback_(cl_event     event ,
                     cl_int       command_exec_callback_type ,
                     void (CL_CALLBACK *  pfn_notify)(cl_event, cl_int, void *),
                     void *       user_data) CL_API_SUFFIX__VERSION_1_1
{
  return CL_INVALID_OPERATION;
}

/* Profiling APIs  */
CL_API_ENTRY cl_int CL_API_CALL
_clGetEventProfilingInfo_(cl_event             event ,
                          cl_profiling_info    param_name ,
                          size_t               param_value_size ,
                          void *               param_value ,
                          size_t *             param_value_size_ret) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clFlush_(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clFinish_(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
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
  *errcode_ret = CL_INVALID_OPERATION;
  return NULL;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueUnmapMemObject_(cl_command_queue  command_queue ,
                          cl_mem            memobj ,
                          void *            mapped_ptr ,
                          cl_uint           num_events_in_wait_list ,
                          const cl_event *   event_wait_list ,
                          cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueTask_(cl_command_queue   command_queue ,
                cl_kernel          kernel ,
                cl_uint            num_events_in_wait_list ,
                const cl_event *   event_wait_list ,
                cl_event *         event) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
}

extern CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueBarrierWithWaitList_(cl_command_queue  command_queue ,
                               cl_uint            num_events_in_wait_list ,
                               const cl_event *   event_wait_list ,
                               cl_event *         event) CL_API_SUFFIX__VERSION_1_2
{
  return CL_INVALID_OPERATION;
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
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueWaitForEvents_(cl_command_queue  command_queue ,
                         cl_uint           num_events ,
                         const cl_event *  event_list) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
}

CL_API_ENTRY cl_int CL_API_CALL
_clEnqueueBarrier_(cl_command_queue  command_queue) CL_API_SUFFIX__VERSION_1_0
{
  return CL_INVALID_OPERATION;
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
