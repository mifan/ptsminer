/*
 * OpenCLObjects.h
 *
 *  Created on: 08/01/2014
 *      Author: girino
 */

#ifndef OPENCLOBJECTS_H_
#define OPENCLOBJECTS_H_

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif

#include <string>
#include <vector>
#include <map>

void check_error(int err_code);

// forward declarations so i can point to parents;
class OpenCLProgram;
class OpenCLContext;
class OpenCLKernel;
class OpenCLDevice;

class OpenCLBuffer {
public:
	OpenCLBuffer(cl_mem _buffer);
	~OpenCLBuffer();
	cl_mem buffer;
};

class OpenCLCommandQueue {
public:
	OpenCLCommandQueue(cl_command_queue _queue);
	~OpenCLCommandQueue();

	cl_event enqueueWriteBuffer(OpenCLBuffer* dest, void* origin, size_t size, cl_event wait_for[] = NULL, size_t num_events = 0);
	cl_event enqueueWriteBufferBlocking(OpenCLBuffer* dest, void* origin, size_t size, cl_event wait_for[] = NULL, size_t num_events = 0);
	cl_event enqueueKernel1D(OpenCLKernel *kernel,	size_t worksize, size_t work_items, cl_event wait_for[],size_t num_events);
	cl_event enqueueReadBuffer(OpenCLBuffer* origin, void* dest, size_t size, cl_event wait_for[] = NULL, size_t num_events = 0);
	cl_event enqueueReadBufferBlocking(OpenCLBuffer* origin, void* dest, size_t size, cl_event wait_for[] = NULL, size_t num_events = 0);

	void finish();
private:
	cl_command_queue queue;
};

class OpenCLKernel {
public:
	OpenCLKernel(cl_kernel _kernel, OpenCLProgram* _parent);
	~OpenCLKernel();

	cl_kernel getKernel();

	void resetArgs();
	void addScalarLong(cl_long arg);
	void addScalarInt(cl_int arg);
	void addScalarULong(cl_ulong arg);
	void addScalarUInt(cl_uint arg);
	void addGlobalArg(OpenCLBuffer* arg);
	void addLocalArg(size_t size);

	// info
	size_t getWorkGroupSize(OpenCLDevice * device);
private:
	cl_kernel kernel;
	// must know its parent;
	OpenCLProgram* parent;
	int arg_count;
};

class OpenCLProgram {
public:
	OpenCLProgram(cl_program _program, OpenCLContext* _parent);
	~OpenCLProgram();

	OpenCLKernel *getKernel(std::string name);
private:
	cl_program program;
	std::map<std::string, OpenCLKernel*> kernels;
	OpenCLContext* parent;
};

class OpenCLDevice {
public:
	OpenCLDevice(cl_device_id _id);
	virtual ~OpenCLDevice();
	std::string getName();
	long getMaxWorkGroupSize();
	long getMaxMemAllocSize();
	long getMaxParamSize();
	int getMaxWorkItemDimensions();
	std::vector<long> getMaxWorkItemSizes();
	cl_device_id getDeviceId();
private:
	cl_device_id my_id;
};

class OpenCLContext {
public:
	OpenCLContext(cl_context _context, std::vector<OpenCLDevice*> _devices);
	~OpenCLContext();

	OpenCLProgram * loadProgramFromFiles(std::vector<std::string> filename);
	OpenCLProgram * loadProgramFromStrings(std::vector<std::string> program);

	OpenCLCommandQueue* createCommandQueue(int deviceIndex=0);
	OpenCLCommandQueue* createCommandQueue(OpenCLDevice* device);

	// buffers are not stored in the context. Algos have the responsibility to dealocate them;
	OpenCLBuffer * createBuffer(size_t size, cl_mem_flags flags=CL_MEM_READ_WRITE, void* original=NULL);

	OpenCLProgram* getProgram(int pos);
private:
	cl_context context;
	std::vector<OpenCLProgram*> programs;
	std::vector<OpenCLDevice*> devices;
};

class OpenCLPlatform {
public:
	OpenCLPlatform(cl_platform_id id, cl_device_type device_type = CL_DEVICE_TYPE_ALL);
	virtual ~OpenCLPlatform();

	OpenCLDevice* getDevice(int pos);
	int getNumDevices();
	OpenCLContext* getContext();
private:
	std::vector<OpenCLDevice *> devices;
	cl_platform_id my_id;
	OpenCLContext* context;
};

class OpenCLMain {
public:
	static OpenCLMain& getInstance() {
        static OpenCLMain instance; // Guaranteed to be destroyed.
        return instance;
	}
	OpenCLPlatform* getPlatform(int pos);
private:
	OpenCLMain();
	virtual ~OpenCLMain();
	// avoid copies
	OpenCLMain(OpenCLMain const&);     // Don't Implement
    void operator=(OpenCLMain const&); // Don't implement

	std::vector<OpenCLPlatform*> platforms;
};

typedef struct _error_struct {
	int err_code;
	std::string err_name;
	std::string err_funcs;
	std::string err_desc;
} error_struct;

// error codes
extern error_struct _errors[];
#endif /* OPENCLOBJECTS_H_ */
