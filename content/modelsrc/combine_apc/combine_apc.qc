// Created by Crowbar 0.71

$ModelName "combine_apc.mdl"

$BodyGroup "Body"
{
	studio "APC_reference.smd"
}

$Include "combine_apc_lod.qci"

$SurfaceProp "metal"

$Contents "solid"

$IllumPosition -5.311 0 68.376

$MostlyOpaque

$CDMaterials "models\combine_apc\"

$Attachment "damage0" "APC.Frame" 0 20 70 rotate 0 0 0
$Attachment "damage1" "APC.Frame" 20 30 -100 rotate 0 0 0
$Attachment "damage2" "APC.Frame" -20 40 -60 rotate 0 0 0
$Attachment "damage3" "APC.Frame" -40 0 80 rotate 0 0 0
$Attachment "gun_base" "APC.Frame" 14.46 45.53 -1.25 rotate -90 -90 0
$Attachment "gun_def" "APC.Gun_Base" 0 6.14 0 rotate -90 -90 0
$Attachment "muzzle" "APC.Gun_Base" -0.11 6.14 52.67 rotate -90 -90 0
$Attachment "cannon_muzzle" "APC.Frame" 0 14.99 58.81 rotate -90 -90 0
$Attachment "wheel_fl" "APC.Wheel_FL_Rotate" 0 0 0 rotate 0 0 0
$Attachment "wheel_fr" "APC.Wheel_FR_Rotate" 0 0 0 rotate 0 0 0
$Attachment "wheel_rl" "APC.Wheel_RL_Rotate" 0 0 0 rotate 0 0 0
$Attachment "wheel_rr" "APC.Wheel_RR_Rotate" 0 0 0 rotate 0 0 0
$Attachment "vehicle_engine" "APC.Frame" 0 0 50 rotate 0 0 0
$attachment "vehicle_feet_passenger1" "APC.Frame" -40.00 -61.72 -0.74 rotate -90 -90 0

$Include "combine_apc_box.qci"

$Include "combine_apc_bone.qci"

$Include "combine_apc_animation.qci"

$Include "combine_apc_collision.qci"

$KeyValues
{
	fire_interactions 
	{
		"flammable"		"yes"
	}
	
	vehicle_npc_passengers 
	{
		"passenger"		 
		{
			"target_attachment"			"vehicle_feet_passenger1"
			"animset"					"apcride_R"
			"entry"			 
			{
				"priority"				"1"
				"animation"				"apcride_R_enter1"
			}
			"exit"			 
			{
				"priority"				"1"
				"animation"				"apcride_R_exit1"
			}
		}
	}
}
