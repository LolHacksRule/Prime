#ifndef __RESSTREAMSFORMAT_H
#define __RESSTREAMSFORMAT_H

//#include "ResStreamsManager.h"

namespace Sexy
{
    class ResStreamsGroup;
    struct ResStreamHeaderV1 //C++ only
    {
        ulong common_data_size;
        ulong file_index_size;
        ulong file_index_location;
        ulong id_index_size;
        ulong id_index_location;
        ulong group_name_to_id_size;
        ulong group_name_to_id_location;
        ulong groups_count;
        ulong groups_location;
        ulong group_header_size;
        ulong pools_count;
        ulong pools_location;
        ulong pool_header_size;
        ulong texture_desc_count;
        ulong texture_desc_location;
        ulong texture_desc_size;
    };
    enum ResStreamDataType //C++ only
    {
        RESSTREAM_RESIDENT,
        RESSTREAM_GPU_TRANSFER,
        RESSTREAM_TRANSIENT
    };
    struct ResStreamHeader //C++ only
    {
        ulong id;
        ulong version_major;
        ulong version_minor;
        ResStreamHeaderV1 v1;
    };
    struct ResStreamFileLocationInfo //C++ only
    {
        ulong res_type;
        ulong location;
        ulong size;
    };
    struct ResStreamFileGPULocationInfo : public ResStreamFileLocationInfo //C++ only
    {
        ulong texture_index;
        ulong atlas_x;
        ulong atlas_y;
        ulong atlas_width;
        ulong atlas_height;
    };
    struct ResStreamGroupHeader //C++ only
    {
        ulong id;
        ulong version_major;
        ulong version_minor;
        ulong total_size;
        ulong data_common_size;
        ulong data_resident_location;
        ulong data_resident_file_size;
        ulong data_resident_mem_size;
        ulong data_gpu_location;
        ulong data_gpu_file_size;
        ulong data_gpu_mem_size;
        ulong data_transient_location;
        ulong data_transient_file_size;
        ulong data_transient_mem_size;
        ulong file_index_size;
        ulong file_index_location;
        ulong texture_desc_count;
        ulong texture_desc_location;
        ulong texture_desc_size;
    };
    struct ResStreamGroupDescriptor //C++ only
    {
        static const ulong MAX_NAME = 0x80;
        uchar name[MAX_NAME];
        ulong file_location;
        ulong file_size;
        ulong pool;
        ulong data_common_size;
        ulong data_resident_location;
        ulong data_resident_file_size;
        ulong data_resident_mem_size;
        ulong data_gpu_location;
        ulong data_gpu_file_size;
        ulong data_gpu_mem_size;
        ulong data_transient_location;
        ulong data_transient_file_size;
        ulong data_transient_mem_size;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
        //V3/+
        ulong textures;
        ulong ;
#endif
    };
    struct ResStreamPoolDescriptor //C++ only
    {
        static const ulong MAX_NAME = 0x80;
        uchar name[MAX_NAME];
        ulong resident_data_size;
        ulong gpu_data_size;
        ulong instance_count;
        ulong flags;
        ulong texture_count;
        ulong texture_offset;
    };
    class ResStreamsPool //C++ only
    {
    public:
        ResStreamsPool();
        ~ResStreamsPool();
        void InitDescriptor(ResStreamPoolDescriptor* theDesc, uchar* theTexDescsPtr, ulong theTexDescSize);
        void Allocate();
        void Destroy();
        bool IsInstanceAvailable();
        ulong LockInstanceForGroup(ResStreamsGroup* theGroup);
        void* GetResidentDataMemory(ulong theInstanceId);
        void* GetGPUDataMemory(ulong theInstanceId);
        void* GetTextureReference(ulong theInstanceId, ulong theTextureId);
        ResStreamTextureDescriptor* GetTextureDescriptor(ulong theTextureId);
        ulong GetNumTextures();
        void UnlockInstanceForGroup(ResStreamsGroup* theGroup);
        const std::string& GetName();
    private:
        std::string mName;
        bool mAllocated;
        ulong mNumInstances;
        ulong mResidentDataSize;
        ulong mGPUDataSize;
        ulong mFlags;
        ResStreamsGroup** mOccupant;
        ulong mNumTexDescs;
        uchar* mTexDescsPtr;
        ulong mTexDescSize;
        struct PoolInstance
        {
            uchar* mResidentData;
            uchar* mGPUData;
            void** mTextures;
        };
        PoolInstance* mInstances;
        uchar* mXboxBigBuffer;
        uchar* mIPhoneBigBuffer;
    };
    struct ResStreamTextureDescriptor //C++ only
    {
        ulong width;
        ulong height;
        ulong pitch;
        ulong format;
#ifdef _SEXYDECOMP_USE_LATEST_CODE
        //PVZ2CN
        //ulong Extend
        //ulong Extend2
#endif
    };
    struct ResStreamsLoadDesc //C++ only
    {
        ResStreamsGroup* group;
        ResStreamsPool* pool;
        ulong instance;
    };
    typedef std::list<ResStreamsLoadDesc> ResStreamsLoadList; //? | Recovered from PDB module
#ifdef _SEXYDECOMP_USE_LATEST_CODE
    struct ResStreamCompositeDescriptor
    {
        static const ulong MAX_NAME = 0x80;
        uchar name[MAX_NAME];
        struct Child
        {
            ulong GroupIndex;
            ulong ArtResolution;
            ulong Localization; //V3/+ only
        };
    };
#endif
}
#endif