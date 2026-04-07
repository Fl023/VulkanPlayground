#pragma once

#include "VulkanDevice.hpp"
#include "VulkanSwapChain.hpp"
#include "core/Layer.hpp"

class ImGuiLayer : public Layer
{
public:
    ImGuiLayer(const VulkanDevice& device, const VulkanSwapChain& swapChain);
    ~ImGuiLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnEvent(Event& e) override;

    void beginFrame();
	void endFrame();

    void BlockEvents(bool block) { m_BlockEvents = block; }

private:
    const VulkanDevice& m_Device;
	const VulkanSwapChain& m_SwapChain;
    VkFormat m_ColorFormat;

    bool m_BlockEvents = true;
};