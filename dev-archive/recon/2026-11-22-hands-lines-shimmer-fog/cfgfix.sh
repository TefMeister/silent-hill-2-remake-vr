# cfgfix.sh: the three starting lines of the profile's config.txt put back (game closed), and the AFW options this session touches
C="$APPDATA/UnrealVRMod/SHProto-Win64-Shipping/config.txt"
sed -i -E 's/^VR_RenderingMethod=.*/VR_RenderingMethod=0/; s/^VR_GhostingFix=.*/VR_GhostingFix=false/; s/^VR_AFW_FramewarpMode=.*/VR_AFW_FramewarpMode=3/; s/^VR_AFW_FixObjectMotionRange=.*/VR_AFW_FixObjectMotionRange=3.000000/; s/^VR_AFW_FixObjectMotionVector=.*/VR_AFW_FixObjectMotionVector=true/; s/^VR_AFW_FixMovingObjectBrightnessFlickering=.*/VR_AFW_FixMovingObjectBrightnessFlickering=true/; s/^VR_AFW_UltraResponsive=.*/VR_AFW_UltraResponsive=true/' "$C"
grep -E "^VR_RenderingMethod|^VR_GhostingFix|^VR_AFW_" "$C" | tr '\n' ' '; echo
