/*
 * Created by v1tr10l7 on 05.07.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
#pragma once

#include <Debug/Assertions.hpp>
#include <Prism/Core/Types.hpp>
#include <Prism/Memory/Pointer.hpp>

// Setup descriptor bit definitions
static constexpr u8 BM_REQUEST_HOST_TO_DEVICE      = (0 << 7);
static constexpr u8 BM_REQUEST_DEVICE_TO_HOST      = (1 << 7);
static constexpr u8 BM_REQUEST_TYPE_STANDARD       = (0 << 5);
static constexpr u8 BM_REQUEST_TYPE_CLASS          = (1 << 5);
static constexpr u8 BM_REQUEST_TYPE_VENDOR         = (2 << 5);
static constexpr u8 BM_REQUEST_TYPE_RESERVED       = (3 << 5);
static constexpr u8 BM_REQUEST_RECIPEINT_DEVICE    = (0 << 0);
static constexpr u8 BM_REQUEST_RECIPIENT_INTERFACE = (1 << 0);
static constexpr u8 BM_REQUEST_RECIPIENT_ENDPOINT  = (2 << 0);
static constexpr u8 BM_REQUEST_RECIPIENT_OTHER     = (3 << 0);

//
// This is also known as the "setup" packet. It's attached to the
// first TD in the chain and is the first piece of data sent to the
// USB device over the bus.
// https://beyondlogic.org/usbnutshell/usb6.shtml#StandardEndpointRequests
//
struct USBRequestData
{
    u8  request_type;
    u8  request;
    u16 value;
    u16 index;
    u16 length;
};

struct [[gnu::packed]] USBDescriptorCommon
{
    u8 length;
    u8 descriptor_type;
};

//
//  Device Descriptor
//  =================
//
//  This descriptor type (stored on the device), represents the device, and
//  gives information related to it, such as the USB specification it complies
//  to, as well as the vendor and product ID of the device.
//
//  https://beyondlogic.org/usbnutshell/usb5.shtml#DeviceDescriptors
struct [[gnu::packed]] USBDeviceDescriptor
{
    USBDescriptorCommon DescriptorHeader;
    u16                 usb_spec_compliance_bcd;
    u8                  device_class;
    u8                  device_sub_class;
    u8                  device_protocol;
    u8                  MaxPacketSize;
    u16                 vendor_id;
    u16                 product_id;
    u16                 device_release_bcd;
    u8                  manufacturer_id_descriptor_index;
    u8                  product_string_descriptor_index;
    u8                  serial_number_descriptor_index;
    u8                  num_configurations;
};

//
//  Configuration Descriptor
//  ========================
//
//  A USB device can have multiple configurations, which tells us about how the
//  device is physically configured (e.g how it's powered, max power consumption
//  etc).
//
struct [[gnu::packed]] USBConfigurationDescriptor
{
    USBDescriptorCommon DescriptorHeader;
    u16                 total_length;
    u8                  number_of_interfaces;
    u8                  configuration_value;
    u8                  configuration_string_descriptor_index;
    u8                  attributes_bitmap;
    u8                  max_power_in_ma;
};

//
//  Interface Descriptor
//  ====================
//
//  An interface descriptor describes to us one or more endpoints, grouped
//  together to define a singular function of a device.
//  As an example, a USB webcam might have two interface descriptors; one
//  for the camera, and one for the microphone.
//
struct [[gnu::packed]] USBInterfaceDescriptor
{
    USBDescriptorCommon DescriptorHeader;
    u8                  interface_id;
    u8                  alternate_setting;
    u8                  number_of_endpoints;
    u8                  interface_class_code;
    u8                  interface_sub_class_code;
    u8                  interface_protocol;
    u8                  interface_string_descriptor_index;
};

//
//  Endpoint Descriptor
//  ===================
//
//  The lowest leaf in the configuration tree. And endpoint descriptor describes
//  the physical transfer properties of the endpoint (that isn't endpoint0).
//  The description given by this structure is used by a pipe to create a
//  "connection" from the host to the device.
//  https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-endpoints-and-their-pipes
struct [[gnu::packed]] USBEndpointDescriptor
{
    USBDescriptorCommon DescriptorHeader;
    u8                  EndpointAddress;
    u8                  EndpointAttributesBitmap;
    u16                 MaxPacketSize;
    u8                  PollIntervalInFrames;
};

static constexpr u8 DESCRIPTOR_TYPE_DEVICE           = 0x01;
static constexpr u8 DESCRIPTOR_TYPE_CONFIGURATION    = 0x02;
static constexpr u8 DESCRIPTOR_TYPE_STRING           = 0x03;
static constexpr u8 DESCRIPTOR_TYPE_INTERFACE        = 0x04;
static constexpr u8 DESCRIPTOR_TYPE_ENDPOINT         = 0x05;
static constexpr u8 DESCRIPTOR_TYPE_DEVICE_QUALIFIER = 0x06;

class Pipe
{
  public:
    enum class Type : u8
    {
        Control     = 0,
        Isochronous = 1,
        Bulk        = 2,
        Interrupt   = 3
    };

    enum class Direction : u8
    {
        Out           = 0,
        In            = 1,
        Bidirectional = 2
    };

    enum class DeviceSpeed : u8
    {
        LowSpeed,
        FullSpeed
    };

  public:
    void      Initialize(Type type, Direction direction, u8 endpointAddress,
                         u16 maxPacketSize, i8 deviceAddress, u8 pollInterval = 0);

    enum Type Type() const { return m_Type; }
    enum Direction   Direction() const { return m_Direction; }
    enum DeviceSpeed DeviceSpeed() const { return m_Speed; }

    i8               DeviceAddress() const { return m_DeviceAddress; }
    u8               EndpointAddress() const { return m_EndpointAddress; }
    u16              MaxPacketSize() const { return m_MaxPacketSize; }
    u8               PollInterval() const { return m_PollInterval; }
    bool             DataToggle() const { return m_DataToggle; }

    void            SetMaxPacketSize(u16 maxSize) { m_MaxPacketSize = maxSize; }
    void            SetToggle(bool toggle) { m_DataToggle = toggle; }
    void            SetDeviceAddress(i8 addr) { m_DeviceAddress = addr; }

    ErrorOr<size_t> ControlTransfer(u8 requestType, u8 request, u16 value,
                                    u16 index, u16 length, void* data);

    Pipe(enum Type type, enum Direction direction, u16 maxPacketSize);
    Pipe(enum Type type, enum Direction direction,
         USBEndpointDescriptor& endpoint);
    Pipe(enum Type type, enum Direction direction, u8 endpointAddress,
         u16 maxPacketSize, u8 pollInterval, i8 deviceAddress);

  private:
    friend class Device;

    enum Type        m_Type;
    enum Direction   m_Direction;
    enum DeviceSpeed m_Speed;

    i8               m_DeviceAddress{0}; // Device address of this pipe
    u8   m_EndpointAddress{0}; // Corresponding endpoint address for this pipe
    u16  m_MaxPacketSize{0};   // Max packet size for this pipe
    u8   m_PollInterval{0};    // Polling interval (in frames)
    bool m_DataToggle{false};  // Data toggle for stuffing bit
};

class USBEndpoint
{
    static constexpr u8 EndpointAddressNumberMask                 = 0x0f;
    static constexpr u8 EndpointAddressDirectionMask              = 0x80;

    static constexpr u8 EndpointAttributesTransferTypeMask        = 0x03;
    static constexpr u8 EndpointAttributesTransferTypeControl     = 0x00;
    static constexpr u8 EndpointAttributesTransferTypeIsochronous = 0x01;
    static constexpr u8 EndpointAttributesTransferTypeBulk        = 0x02;
    static constexpr u8 EndpointAttributesTransferTypeInterrupt   = 0x03;

    static constexpr u8 EndpointAttributesIsoModeSyncType         = 0x0c;
    static constexpr u8 EndpointAttributesIsoModeUsageType        = 0x30;

  public:
    const USBEndpointDescriptor& Descriptor() const { return m_Descriptor; }

    bool                         IsControl() const
    {
        return (m_Descriptor.EndpointAttributesBitmap
                & EndpointAttributesTransferTypeMask)
            == EndpointAttributesTransferTypeControl;
    }
    bool IsIsochronous() const
    {
        return (m_Descriptor.EndpointAttributesBitmap
                & EndpointAttributesTransferTypeMask)
            == EndpointAttributesTransferTypeIsochronous;
    }
    bool IsBulk() const
    {
        return (m_Descriptor.EndpointAttributesBitmap
                & EndpointAttributesTransferTypeMask)
            == EndpointAttributesTransferTypeBulk;
    }
    bool IsInterrupt() const
    {
        return (m_Descriptor.EndpointAttributesBitmap
                & EndpointAttributesTransferTypeMask)
            == EndpointAttributesTransferTypeInterrupt;
    }

    u16 MaxPacketSize() const { return m_Descriptor.MaxPacketSize; }
    u8  PollingInterval() const { return m_Descriptor.PollIntervalInFrames; }

  private:
    USBEndpoint(/* TODO */);
    USBEndpointDescriptor m_Descriptor;

    Pipe                  m_Pipe;
};

class Transfer
{
  public:
    void                  Initialize(Pipe* pipe, u16 len);

    void                  SetSetupPacket(const USBRequestData& request);
    void                  SetComplete() { m_Complete = true; }
    void                  SetErrorOccurred() { m_ErrorOccurred = true; }

    // `const` here makes sure we don't blow up by writing to a physical address
    const USBRequestData& Request() const { return m_Request; }
    const class Pipe&     Pipe() const { return *m_Pipe; }
    class Pipe&           Pipe() { return *m_Pipe; }
    usize                 Buffer() const { return m_DataBuffer.Raw(); }
    usize BufferPhysical() const { return m_DataBuffer.FromHigherHalf(); }
    u16   TransferDataSize() const { return m_TransferDataSize; }
    bool  Complete() const { return m_Complete; }
    bool  ErrorOccurred() const { return m_ErrorOccurred; }

  private:
    class Pipe*    m_Pipe;                // Pipe that initiated this transfer
    USBRequestData m_Request;             // USB request
    Pointer        m_DataBuffer;          // DMA Data buffer for transaction
    u16            m_TransferDataSize{0}; // Size of the transfer's data stage
    bool           m_Complete{false};     // Has this transfer been completed?
    bool m_ErrorOccurred{false}; // Did an error occur during this transfer?
};

template <typename T>
class Ptr32
{
  public:
    constexpr Ptr32() = default;
    Ptr32(T* const ptr)
        : m_ptr((u32) reinterpret_cast<u64>(ptr))
    {
        Assert((reinterpret_cast<u64>(ptr) & 0xFFFFFFFFULL)
               == static_cast<u64>(m_ptr));
    }
    T&       operator*() { return *static_cast<T*>(*this); }
    T const& operator*() const { return *static_cast<T const*>(*this); }

    T*       operator->() { return *this; }
    T const* operator->() const { return *this; }

    operator T*() { return reinterpret_cast<T*>(static_cast<u64>(m_ptr)); }
    operator T const*() const
    {
        return reinterpret_cast<T const*>(static_cast<u64>(m_ptr));
    }

    T&       operator[](size_t index) { return static_cast<T*>(*this)[index]; }
    T const& operator[](size_t index) const
    {
        return static_cast<T const*>(*this)[index];
    }

    constexpr explicit operator bool() { return m_ptr; }
    template <typename U>
    constexpr bool operator==(Ptr32<U> other)
    {
        return m_ptr == other.m_ptr;
    }

    constexpr Ptr32<T> operator+(u32 other) const
    {
        Ptr32<T> ptr{};
        ptr.m_ptr = m_ptr + other;
        return ptr;
    }
    constexpr Ptr32<T> operator-(u32 other) const
    {
        Ptr32<T> ptr{};
        ptr.m_ptr = m_ptr - other;
        return ptr;
    }

  private:
    u32 m_ptr{0};
};

namespace USB::UHCI
{

    enum class PacketID : u8
    {
        IN    = 0x69,
        OUT   = 0xe1,
        SETUP = 0x2d
    };

    // Transfer Descriptor register bit offsets/masks
    constexpr u16 TD_CONTROL_STATUS_ACTLEN                = 0x7ff;
    constexpr u8  TD_CONTROL_STATUS_ACTIVE_SHIFT          = 23;
    constexpr u8  TD_CONTROL_STATUS_INT_ON_COMPLETE_SHIFT = 24;
    constexpr u8  TD_CONTROL_STATUS_ISOCHRONOUS_SHIFT     = 25;
    constexpr u8  TD_CONTROL_STATUS_LS_DEVICE_SHIFT       = 26;
    constexpr u8  TD_CONTROL_STATUS_ERR_CTR_SHIFT_SHIFT   = 27;
    constexpr u8  TD_CONTROL_STATUS_SPD_SHIFT             = 29;

    constexpr u8  TD_TOKEN_PACKET_ID_SHIFT                = 0;
    constexpr u8  TD_TOKEN_DEVICE_ADDR_SHIFT              = 8;
    constexpr u8  TD_TOKEN_ENDPOINT_SHIFT                 = 15;
    constexpr u8  TD_TOKEN_DATA_TOGGLE_SHIFT              = 19;
    constexpr u8  TD_TOKEN_MAXLEN_SHIFT                   = 21;

    //
    // Transfer Descriptor
    //
    // Describes a single transfer event from, or to the Universal Serial Bus.
    // These are, generally, attached to Queue Heads, and then executed by the
    // USB Host Controller.
    // Must be 16-byte aligned
    //
    struct QueueHead;
    struct alignas(16) TransferDescriptor final
    {
        enum LinkPointerBits
        {
            eTerminate = 1,
            QHSelect   = 2,
            DepthFlag  = 4,
        };

        enum StatusBits
        {
            Reserved        = (1 << 16),
            BitStuffError   = (1 << 17),
            CRCTimeoutError = (1 << 18),
            NAKReceived     = (1 << 19),
            BabbleDetected  = (1 << 20),
            DataBufferError = (1 << 21),
            eStalled        = (1 << 22),
            eActive         = (1 << 23),
            ErrorMask       = BitStuffError | CRCTimeoutError | NAKReceived
                      | BabbleDetected | DataBufferError | eStalled
        };

        enum ControlBits
        {
            InterruptOnComplete = (1 << 24),
            IsochronousSelect   = (1 << 25),
            LowSpeedDevice      = (1 << 26),
            ShortPacketDetect   = (1 << 29),
        };

        TransferDescriptor() = delete;
        TransferDescriptor(u32 phys)
            : m_Phys(phys)
        {
        }
        ~TransferDescriptor()
            = delete; // Prevent anything except placement new on this object

        u32 Link() const { return m_Link; }
        u32 Phys() const { return m_Phys; }
        u32 Status() const { return m_ControlStatus; }
        u32 Token() const { return m_Token; }
        u32 Buffer() const { return m_Buffer; }
        u16 ActualPackageLength() const
        {
            return (m_ControlStatus + 1) & 0x7ff;
        }

        bool InUse() const { return m_InUse; }
        bool Stalled() const { return m_ControlStatus & StatusBits::eStalled; }
        bool LastInChain() const
        {
            return m_Link & LinkPointerBits::eTerminate;
        }
        bool Active() const { return m_ControlStatus & StatusBits::eActive; }

        void SetActive()
        {
            u32 ctrl = m_ControlStatus;
            ctrl |= StatusBits::eActive;
            m_ControlStatus = ctrl;
        }

        void SetIsochronous()
        {
            u32 ctrl = m_ControlStatus;
            ctrl |= ControlBits::IsochronousSelect;
            m_ControlStatus = ctrl;
        }

        void SetInterruptOnComplete()
        {
            u32 ctrl = m_ControlStatus;
            ctrl |= ControlBits::InterruptOnComplete;
            m_ControlStatus = ctrl;
        }

        void SetLowSpeed()
        {
            u32 ctrl = m_ControlStatus;
            ctrl |= ControlBits::LowSpeedDevice;
            m_ControlStatus = ctrl;
        }

        void SetErrorRetryCounter(u8 retries)
        {
            Assert(retries <= 3);
            u32 ctrl = m_ControlStatus;
            ctrl |= (retries << 27);
            m_ControlStatus = ctrl;
        }

        void SetShortPacketDetect()
        {
            u32 ctrl = m_ControlStatus;
            ctrl |= ControlBits::ShortPacketDetect;
            m_ControlStatus = ctrl;
        }

        void SetControlStatus(u32 controlStatus)
        {
            m_ControlStatus = controlStatus;
        }
        void SetInUse(bool inUse) { m_InUse = inUse; }
        void SetMaxLen(u16 maxLen)
        {
            Assert(maxLen < 0x500 || maxLen == 0x7ff);
            m_Token |= (maxLen << 21);
        }

        void SetDeviceEndpoint(u8 endpoint)
        {
            Assert(endpoint <= 0xf);
            m_Token |= (endpoint << 18);
        }

        void SetDeviceAddress(u8 address)
        {
            Assert(address <= 0x7f);
            m_Token |= (address << 8);
        }

        void SetDataToggle(bool toggle)
        {
            m_Token |= ((toggle ? (1 << 19) : 0));
        }

        void SetPacketID(PacketID pid) { m_Token |= static_cast<u32>(pid); }
        void LinkQueueHead(u32 qhPhys)
        {
            m_Link = qhPhys;
            m_Link |= LinkPointerBits::QHSelect;
        }

        // FIXME: For the love of God, use AK SMART POINTERS PLEASE!!
        TransferDescriptor*       NextTd() { return m_NextTd; }
        const TransferDescriptor* NextTd() const { return m_NextTd; }
        void                SetNextTd(TransferDescriptor* td) { m_NextTd = td; }

        TransferDescriptor* PrevTd() { return m_PrevTd; }
        const TransferDescriptor* PrevTd() const { return m_PrevTd; }
        void SetPreviousTd(TransferDescriptor* td) { m_PrevTd = td; }

        void InsertNextTransferDescriptor(TransferDescriptor* td)
        {
            m_Link = td->Phys();
            td->SetPreviousTd(this);
            SetNextTd(td);

            // Let's set some bits for the link ptr
            m_Link |= static_cast<u32>(LinkPointerBits::DepthFlag);
        }

        void Terminate()
        {
            m_Link |= static_cast<u32>(LinkPointerBits::eTerminate);
        }

        void SetBufferAddress(Ptr32<u8> buffer)
        {
            u8* bufferAddress = &*buffer;
            m_Buffer          = reinterpret_cast<uintptr_t>(bufferAddress);
        }

        // DEBUG FUNCTIONS!
        void SetToken(u32 token) { m_Token = token; }

        void SetStatus(u32 status) { m_ControlStatus = status; }

        void Free()
        {
            m_Link          = 0;
            m_ControlStatus = 0;
            m_Token         = 0;
            m_InUse         = false;
        }

      private:
        u32 m_Link; // Points to another Queue Head or Transfer Descriptor
        volatile u32 m_ControlStatus; // Control and status bits
        u32 m_Token; // Contains all information required to fill in a USB Start
                     // Token
        u32 m_Buffer; // Points to a data buffer for this transaction (i.e
                      // what we want to send or recv)

        // These values will be ignored by the controller, but we can use them
        // for configuration/bookkeeping
        u32 m_Phys; // Physical address where this TransferDescriptor is
                    // located
        Ptr32<TransferDescriptor> m_NextTd{nullptr}; // Pointer to first TD
        Ptr32<TransferDescriptor> m_PrevTd{nullptr}; // Pointer to first TD
        bool m_InUse; // Has this TD been allocated (and therefore in use)?
    };

    static_assert(sizeof(TransferDescriptor)
                  == 32); // Transfer Descriptor is always 8 Dwords

    //
    // Queue Head
    //
    // Description here please!
    //
    struct alignas(16) QueueHead
    {
        enum class LinkPointerBits : u32
        {
            Terminate = 1,
            QHSelect  = 2,
        };

        QueueHead() = delete;
        QueueHead(u32 paddr)
            : m_Phys(paddr)
        {
        }
        ~QueueHead()
            = delete; // Prevent anything except placement new on this object

        u32              Link() const { return m_Link; }
        u32              ElementLink() const { return m_ElementLink; }
        u32              Phys() const { return m_Phys; }
        bool             InUse() const { return m_InUse; }

        void             SetInUse(bool in_use) { m_InUse = in_use; }
        void             SetLink(u32 val) { m_Link = val; }

        // FIXME: For the love of God, use AK SMART POINTERS PLEASE!!
        QueueHead*       NextQh() { return m_NextQh; }
        const QueueHead* NextQh() const { return m_NextQh; }
        void             SetNextQh(QueueHead* qh) { m_NextQh = qh; }

        QueueHead*       PrevQh() { return m_PrevQh; }
        const QueueHead* PrevQh() const { return m_PrevQh; }
        void             SetPreviousQh(QueueHead* qh) { m_PrevQh = qh; }

        void             LinkNextQueueHead(QueueHead* qh)
        {
            m_Link = qh->Phys();
            m_Link |= static_cast<u32>(LinkPointerBits::QHSelect);
        }

        void AttachTransferQueue(QueueHead& qh)
        {
            m_ElementLink = qh.Phys();
            m_ElementLink
                = m_ElementLink | static_cast<u32>(LinkPointerBits::QHSelect);
        }

        // FIXME: Find out best way to walk queue and free everything
        void FreeTransferQueue([[maybe_unused]] QueueHead* qh) { ; }

        void TerminateWithStrayDescriptor(TransferDescriptor* td)
        {
            m_Link = td->Phys();
            m_Link |= static_cast<u32>(LinkPointerBits::Terminate);
        }

        // TODO: Should we pass in an array or vector of TDs instead????
        void AttachTransferDescriptorChain(TransferDescriptor* td)
        {
            m_FirstTd     = td;
            m_ElementLink = td->Phys();
        }

        TransferDescriptor* GetFirstTd() { return m_FirstTd; }

        void                Terminate()
        {
            m_Link |= static_cast<u32>(LinkPointerBits::Terminate);
        }

        void TerminateElementLink()
        {
            m_ElementLink = static_cast<u32>(LinkPointerBits::Terminate);
        }

        void      SetTransfer(Transfer* transfer) { m_Transfer = transfer; }

        Transfer* Transfer() { return m_Transfer; }

        void      Free()
        {
            m_Link        = 0;
            m_ElementLink = 0;
            m_FirstTd     = nullptr;
            m_Transfer    = nullptr;
            m_InUse       = false;
        }

      private:
        u32 m_Link{0}; // Pointer to the next horizontal object that the
                       // controller will execute after this one
        volatile u32 m_ElementLink{0}; // Pointer to the first data object in
                                       // the queue (can be modified by hw)

        // These values will be ignored by the controller, but we can use them
        // for configuration/bookkeeping Any addresses besides `paddr` are
        // assumed virtual and can be dereferenced
        u32 m_Phys{0}; // Physical address where this QueueHead is located
        Ptr32<QueueHead>          m_NextQh{nullptr};  // Next QH
        Ptr32<QueueHead>          m_PrevQh{nullptr};  // Previous QH
        Ptr32<TransferDescriptor> m_FirstTd{nullptr}; // Pointer to first TD
        [[maybe_unused]] Ptr32<class Transfer> m_Transfer{
            nullptr};        // Pointer to transfer linked to this queue head
        bool m_InUse{false}; // Is this QH currently in use?
    };

    struct [[gnu::packed]] FrameListEntry
    {
        bool Empty           : 1;
        bool QueueHead       : 1;
        u8   Reserved        : 2;
        u32  PhysicalAddress : 28;
    };
    // struct [[gnu::packed]] QueueHead
    // {
    //     u32 HorizontalPointer = 0;
    //     u32 VerticalPointer   = 0;
    // };
    // struct [[gnu::packed]] TransferDescriptor
    // {
    //     u32 NextDescriptor = 0;
    //     u32 Status         = 0;
    //     u32 PacketHeader   = 0;
    //     u32 BufferAddress  = 0;
    //     u32 SystemUse      = 0;
    // };
    struct [[gnu::packed]] TransferDescriptorNextDescriptor
    {
        u32  PhysicalAddress : 28;
        bool Reserved        : 1;
        bool DepthFirst      : 1;
        bool QueueHead       : 1;
        bool Terminate       : 1;
    };
    struct [[gnu::packed]] TransferDescriptorStatus
    {
        u8   Reserved            : 2;
        bool ShortPacketDetect   : 1;
        u8   ErrorCounter        : 2;
        bool LowSpeed            : 1;
        bool IsIsochronous       : 1;
        bool InterruptOnComplete : 1;
        bool Active              : 1;
        bool Stalled             : 1;
        bool DataBufferError     : 1;
        bool BabbleDetected      : 1;
        bool NonAcknowledged     : 1;
        bool TimeoutCRC          : 1;
        bool BitStuffError       : 1;
        u16  Reserved2           : 6;
        u16  ActualLength        : 11;
    };
    struct [[gnu::packed]] TransferDescriptorPacketHeader
    {
        u16  MaximumLength : 11;
        bool Reserved      : 1;
        bool DataToggle    : 1;
        u8   Endpoint      : 4;
        u8   Device        : 7;
        u8   PacketType;
    };
}; // namespace USB::UHCI
