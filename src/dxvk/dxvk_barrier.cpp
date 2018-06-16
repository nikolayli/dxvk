#include "dxvk_barrier.h"

namespace dxvk {
  
  DxvkBarrierSet:: DxvkBarrierSet() { }
  DxvkBarrierSet::~DxvkBarrierSet() { }
  
  void DxvkBarrierSet::accessBuffer(
    const DxvkPhysicalBufferSlice&  bufSlice,
          VkPipelineStageFlags      srcStages,
          VkAccessFlags             srcAccess,
          VkPipelineStageFlags      dstStages,
          VkAccessFlags             dstAccess) {
    DxvkAccessFlags access = this->getAccessTypes(srcAccess);
    
    m_srcStages |= srcStages;
    m_dstStages |= dstStages;
    
    if (access.test(DxvkAccess::Write)) {
      VkBufferMemoryBarrier barrier;
      barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
      barrier.pNext               = nullptr;
      barrier.srcAccessMask       = srcAccess;
      barrier.dstAccessMask       = dstAccess;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.buffer              = bufSlice.handle();
      barrier.offset              = bufSlice.offset();
      barrier.size                = bufSlice.length();
      m_bufBarriers.push_back(barrier);
    }

    m_bufSlices.push_back({ bufSlice, access });
  }
  
  
  void DxvkBarrierSet::accessImage(
    const Rc<DxvkImage>&            image,
    const VkImageSubresourceRange&  subresources,
          VkImageLayout             srcLayout,
          VkPipelineStageFlags      srcStages,
          VkAccessFlags             srcAccess,
          VkImageLayout             dstLayout,
          VkPipelineStageFlags      dstStages,
          VkAccessFlags             dstAccess) {
    DxvkAccessFlags access = this->getAccessTypes(srcAccess);
    
    m_srcStages |= srcStages;
    m_dstStages |= dstStages;
    
    if ((srcLayout != dstLayout) || access.test(DxvkAccess::Write)) {
      VkImageMemoryBarrier barrier;
      barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.pNext                       = nullptr;
      barrier.srcAccessMask               = srcAccess;
      barrier.dstAccessMask               = dstAccess;
      barrier.oldLayout                   = srcLayout;
      barrier.newLayout                   = dstLayout;
      barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
      barrier.image                       = image->handle();
      barrier.subresourceRange            = subresources;
      barrier.subresourceRange.aspectMask = image->formatInfo()->aspectMask;
      m_imgBarriers.push_back(barrier);
    }
  }
  
  
  bool DxvkBarrierSet::isBufferDirty(
    const DxvkPhysicalBufferSlice&  bufSlice,
          DxvkAccessFlags           bufAccess) {
    bool result = false;

    for (uint32_t i = 0; i < m_bufSlices.size() && !result; i++) {
      result = (bufSlice.overlaps(m_bufSlices[i].slice))
            && (bufAccess | m_bufSlices[i].access).test(DxvkAccess::Write);
    }

    return result;
  }


  void DxvkBarrierSet::recordCommands(const Rc<DxvkCommandList>& commandList) {
    if ((m_srcStages | m_dstStages) != 0) {
      VkPipelineStageFlags srcFlags = m_srcStages;
      VkPipelineStageFlags dstFlags = m_dstStages;
      
      if (srcFlags == 0) srcFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      if (dstFlags == 0) dstFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
      
      commandList->cmdPipelineBarrier(
        srcFlags, dstFlags, 0,
        m_memBarriers.size(), m_memBarriers.data(),
        m_bufBarriers.size(), m_bufBarriers.data(),
        m_imgBarriers.size(), m_imgBarriers.data());
      
      this->reset();
    }
  }
  
  
  void DxvkBarrierSet::reset() {
    m_srcStages = 0;
    m_dstStages = 0;
    
    m_memBarriers.resize(0);
    m_bufBarriers.resize(0);
    m_imgBarriers.resize(0);

    m_bufSlices.resize(0);
  }
  
  
  DxvkAccessFlags DxvkBarrierSet::getAccessTypes(VkAccessFlags flags) const {
    const VkAccessFlags rflags
      = VK_ACCESS_INDIRECT_COMMAND_READ_BIT
      | VK_ACCESS_INDEX_READ_BIT
      | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
      | VK_ACCESS_UNIFORM_READ_BIT
      | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT
      | VK_ACCESS_SHADER_READ_BIT
      | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
      | VK_ACCESS_TRANSFER_READ_BIT
      | VK_ACCESS_HOST_READ_BIT
      | VK_ACCESS_MEMORY_READ_BIT;
      
    const VkAccessFlags wflags
      = VK_ACCESS_SHADER_WRITE_BIT
      | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
      | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
      | VK_ACCESS_TRANSFER_WRITE_BIT
      | VK_ACCESS_HOST_WRITE_BIT
      | VK_ACCESS_MEMORY_WRITE_BIT;
    
    DxvkAccessFlags result;
    if (flags & rflags) result.set(DxvkAccess::Read);
    if (flags & wflags) result.set(DxvkAccess::Write);
    return result;
  }
  
}