// Options, shader constants and dev-menu controls for the cheaper visibility rays.
// See rtx_fork_shadow_rays.h.

#include "rtx_fork_shadow_rays.h"

#include <algorithm>

#include "rtx_imgui.h"
#include "rtx_options.h"
#include "rtx_pre_exposure.h"
#include "rtx/pass/raytrace_args.h"

namespace dxvk {

  void ShadowRays::setRaytraceArgs(RaytraceArgs& args, bool nrdDenoises) {
    // NRD is the only reader of the primary direct hit distance, and it is the only thing first-hit changes.
    // Bit 0: the primary NEE ray. Bit 1: the subsurface-path shadow rays (no reader of their hit distance).
    // Bit 2: the POM self-shadow march after the scene ray. It only changes the hit distance of rays that both block.
    args.shadowRayFirstHitPrimaryNee = ((firstHitPrimaryNee() && !nrdDenoises) ? 1u : 0u) |
                                       (firstHitSubsurface() ? 2u : 0u) |
                                       ((pomAfterSceneRay() && !nrdDenoises) ? 4u : 0u) |
                                       (directLeanPath() ? 8u : 0u) |
                                       (indirectCutoutOpacityFirst() ? 16u : 0u);

    // Radiance is stored times the pre-exposure P. The tonemapper multiplies by trackedExposure / P, so a value on the
    // display-linear scale is that value times P / trackedExposure in stored radiance.
    const float displayToRadiance = PreExposure::get() / std::max(PreExposure::trackedExposure(), 1e-30f);
    const bool cutoffActive = contributionCutoff() >= 0.0f && !nrdDenoises && !RtxOptions::useDenoiserReferenceMode();
    args.shadowRayDisplayToRadiance = displayToRadiance;
    args.shadowRayContributionCutoff = cutoffActive ? contributionCutoff() * displayToRadiance : -1.0f;
    args.shadowRayContributionCutoffSunRelative = cutoffActive ? std::max(contributionCutoffSunRelative(), 0.0f) : 0.0f;
    // Only with SHaRC. The other indirect modes reuse a path's radiance on other pixels with other first-bounce throughputs.
    args.shadowRayContributionCutoffIndirect = (cutoffActive && contributionCutoffIndirectScale() >= 0.0f &&
                                                RtxOptions::integrateIndirectMode() == IntegrateIndirectMode::Sharc)
      ? contributionCutoff() * contributionCutoffIndirectScale() * displayToRadiance : -1.0f;

    const uint32_t targetResolution = std::clamp(alphaMipTargetResolution(), 4u, 4096u);
    args.shadowRayAlphaGradient = coarseAlphaMip() ? 1.0f / static_cast<float>(targetResolution) : 0.0f;
  }

  void ShadowRays::showImguiSettings() {
    if (RemixGui::CollapsingHeader("Shadow / Visibility Rays", ImGuiTreeNodeFlags_CollapsingHeader)) {
      ImGui::Indent();
      ImGui::PushID("ShadowRays");

      RemixGui::Checkbox("Primary NEE Shadow Ray Stops at First Hit", &firstHitPrimaryNeeObject());
      RemixGui::Checkbox("Subsurface Shadow Rays Stop at First Hit", &firstHitSubsurfaceObject());
      RemixGui::Checkbox("Direct Lighting: Material Before the Rays", &directLeanPathObject());
      RemixGui::Checkbox("Height Map Self-Shadow After Scene Ray", &pomAfterSceneRayObject());
      RemixGui::Checkbox("Indirect Cutout: Opacity Before the Material", &indirectCutoutOpacityFirstObject());
      RemixGui::DragFloat("Skip Shadow Rays Below (display-linear)", &contributionCutoffObject(), 0.00001f, -1.0f, 0.01f, "%.5f");
      RemixGui::DragFloat("Skip Below Fraction of Sun Contribution", &contributionCutoffSunRelativeObject(), 0.0005f, 0.0f, 0.05f, "%.4f");
      RemixGui::DragFloat("Indirect Scale of the Skip Value", &contributionCutoffIndirectScaleObject(), 0.05f, -1.0f, 10.0f, "%.2f");
      RemixGui::Checkbox("Coarse Mip for Visibility Alpha Tests", &coarseAlphaMipObject());
      ImGui::BeginDisabled(!coarseAlphaMip());
      RemixGui::DragInt("Alpha Test Mip Resolution (texels)", &alphaMipTargetResolutionObject(), 1.f, 4, 4096);
      ImGui::EndDisabled();

      ImGui::PopID();
      ImGui::Unindent();
    }
  }

} // namespace dxvk
