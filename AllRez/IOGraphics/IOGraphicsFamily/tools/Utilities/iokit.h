// vim: set ft=cpp:
//
//  IOGDiagnoseUtils/iokit
//  IOGDiagnoseUtils
//
//  Created by Godfrey van der Linden 2018-10-31
//

#ifndef IOGDIAGNOSEUTILS_IOKIT
#define IOGDIAGNOSEUTILS_IOKIT

#if defined(IOGD530_66)
#else
#include <utility>
#endif

#include <IOKit/IOKitLib.h>

class IOObject
{
    // Takes ownership of IOKit registry object
    mutable io_object_t fObject = MACH_PORT_NULL;
    void retain()  const {
        if (fObject)
            IOObjectRetain(fObject);
    }
    void release() const {
        if (fObject) {
            IOObjectRelease(fObject);
            fObject = MACH_PORT_NULL;
        }
    }
public:
    // Takes ownership of IOKit user client connection
    IOObject(io_object_t object) : fObject(object) { }
    ~IOObject() { release(); }
    // Takes ownership of underlying connect, will release at end of scope
    IOObject& operator=(const io_object_t& object)
    {
        release();
        fObject = object;
        return *this;
    }

    // Copy support
    IOObject(const IOObject& other)
    {
        release();
        other.retain();
        fObject = other.fObject;
    }
    IOObject& operator=(const IOObject& other)
    {
        if (this != &other) {
            release();
            other.retain();
            fObject = other.fObject;
        }
        return *this;
    }

    // Move support
    IOObject(IOObject&& other)
    {
        if (this != &other) {
            release();
            std::swap(fObject, other.fObject);
        }
    }
    IOObject& operator=(IOObject&& other)
    {
        if (this != &other) {
            release();
            std::swap(fObject, other.fObject);
        }
        return *this;
    }
    io_object_t get() const { return fObject; }
    operator bool() const { return static_cast<bool>(get()); }
};

class IOConnect
{
    kern_return_t fLastErr = kIOReturnSuccess;
    io_connect_t fConnect = MACH_PORT_NULL;
    void release()
    {
        if (fConnect) {
            IOServiceClose(fConnect);
            fConnect = MACH_PORT_NULL;
        }
        fLastErr = kIOReturnSuccess;
    }
    kern_return_t open(const io_object_t o, const uint32_t t, io_connect_t* cp)
        { return IOServiceOpen(o, mach_task_self(), t, cp); }
public:
    // Takes ownership of IOKit user client connection
    IOConnect(io_object_t object, uint32_t type)      // Open constructor
        { fLastErr = open(object, type, &fConnect); }
    IOConnect(const IOObject& object, uint32_t type)  // Open constructor
        { fLastErr = open(object.get(), type, &fConnect); }
    explicit IOConnect(io_connect_t connect) : fConnect(connect) {}
    IOConnect() {}
    ~IOConnect() { release(); }
    IOConnect(IOConnect&& other)
    {
        if (this != &other) {
            release();
            std::swap(fLastErr, other.fLastErr);
            std::swap(fConnect, other.fConnect);
        }
    }
    IOConnect& operator=(IOConnect&& other)
    {
        if (this != &other) {
            release();
            std::swap(fLastErr, other.fLastErr);
            std::swap(fConnect, other.fConnect);
        }
        return *this;
    }

    // Takes ownership of underlying connect, will close at end of scope
    IOConnect& operator=(const io_connect_t& connect)
    {
        if (fConnect != connect) {
            release();
            fConnect = connect;
        }
        return *this;
    }

    // No valid copy semantic for io_connect_t as IOServiceClose is not
    // reference counted thus the first close will destroy the connection.
    IOConnect(const IOConnect& other) = delete;
    IOConnect& operator=(const IOConnect& other) = delete;

    io_connect_t get() const   { return fConnect; }
    kern_return_t err() const  { return fLastErr; }
    operator bool() const      { return static_cast<bool>(get()); }

    kern_return_t callMethod(uint32_t        selector,    // In
	                         const uint64_t* in,          // In
	                         uint32_t        inCnt,       // In
	                         const void*     inBlob,      // In
	                         size_t          inBlobCnt,   // In
	                         uint64_t*       out,         // Out
	                         uint32_t*       outCnt,      // In/Out
	                         void*           outBlob,     // Out
	                         size_t*         outBlobCntP) // In/Out
        const
    {
        return IOConnectCallMethod(fConnect, selector,
                                   in, inCnt, inBlob, inBlobCnt,
                                   out, outCnt, outBlob, outBlobCntP);
    }
};

#endif // !IOGDIAGNOSEUTILS_IOKIT
