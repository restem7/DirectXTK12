//--------------------------------------------------------------------------------------
// File: GraphicsMemory.h
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

#pragma once

#if defined(_XBOX_ONE) && defined(_TITLE)
#include <d3d12_x.h>
#else
#include <d3d12.h>
#endif


namespace DirectX
{
    class LinearAllocatorPage;

    // Works a little like a smart pointer. The memory will only be fenced by the GPU once the pointer
    // has been invalidated or the user explicitly marks it for fencing.
    class GraphicsResource
    {
    public:
        GraphicsResource();
        GraphicsResource(
            _In_ LinearAllocatorPage* page,
            _In_ D3D12_GPU_VIRTUAL_ADDRESS gpuAddress,
            _In_ ID3D12Resource* resource,
            _In_ void* memory, 
            _In_ size_t offset,
            _In_ size_t size);

        GraphicsResource(GraphicsResource&& other);
        GraphicsResource&& operator= (GraphicsResource&&);

        GraphicsResource(const GraphicsResource&) = delete;
        GraphicsResource& operator= (const GraphicsResource&) = delete;

        ~GraphicsResource();

        D3D12_GPU_VIRTUAL_ADDRESS GpuAddress() const { return mGpuAddress; }
        ID3D12Resource* Resource() const { return mResource; }
        void* Memory() const { return mMemory; }
        size_t ResourceOffset() const { return mBufferOffset; }
        size_t Size() const { return mSize; }

        // Clear the pointer. Using operator -> will produce bad results.
        void __cdecl Reset();
        void __cdecl Reset(GraphicsResource&&);

    private:
        LinearAllocatorPage*        mPage;
        D3D12_GPU_VIRTUAL_ADDRESS   mGpuAddress;
        ID3D12Resource*             mResource;
        void*                       mMemory;
        size_t                      mBufferOffset;
        size_t                      mSize;
    };

    class GraphicsMemory
    {
    public:
        explicit GraphicsMemory(_In_ ID3D12Device* device);

        GraphicsMemory(GraphicsMemory&& moveFrom);
        GraphicsMemory& operator= (GraphicsMemory&& moveFrom);

        GraphicsMemory(GraphicsMemory const&) = delete;
        GraphicsMemory& operator=(GraphicsMemory const&) = delete;

        virtual ~GraphicsMemory();

        // Make sure to keep the GraphicsResource handle alive as long as you need to access
        // the memory on the CPU. For example, do not simply cache GpuAddress() and discard
        // the GraphicsResource object, or your memory may be overwritten later.
        GraphicsResource __cdecl Allocate(size_t size, size_t alignment = 0);

        // Special overload of Allocate that aligns to D3D12 constant buffer alignment requirements
        template<typename T> GraphicsResource __cdecl AllocateConstant()
        {
            const size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
            const size_t alignedSize = (sizeof(T) + alignment - 1) & ~(alignment - 1);
            return std::move(Allocate(alignedSize, alignment));
        }
        template<typename T> GraphicsResource __cdecl AllocateConstant(const T& setData)
        {
            GraphicsResource alloc = AllocateConstant<T>();
            memcpy(alloc.Memory(), &setData, sizeof(T));
            return std::move(alloc);
        }

        // Submits all the pending one-shot memory to the GPU. 
        // The memory will be recycled once the GPU is done with it.
        void __cdecl Commit(_In_ ID3D12CommandQueue* commandQueue);

        // This frees up any unused memory. 
        // If you want to make sure all memory is reclaimed, idle the GPU before calling this.
        // It is not recommended that you call this unless absolutely necessary (e.g. your
        // memory budget changes at run-time, or perhaps you're changing levels in your game.)
        void __cdecl GarbageCollect();

        // Singleton
        static GraphicsMemory& __cdecl Get();

    private:
        // Private implementation.
        class Impl;

        std::unique_ptr<Impl> pImpl;
    };
}
