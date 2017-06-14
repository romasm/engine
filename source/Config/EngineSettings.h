#pragma once

#include "Pathes.h"
#include "FileIO.h"

#define CONFIG(option) EngineSettings::EngSets.option

namespace EngineCore
{
	class EngineSettings
	{
	public:
		
		static struct ESet
		{
			bool fullscreen;
			bool vsync;
			bool win_border;
			int wres;
			int hres;
			int fpslock;

			wstring localization;

			bool msaa;
			int msaa_counts;
			int msaa_quality;

			int texfilter_quality;
			float mipmap_offset;

			bool hud_fps;

			float cam_near_clip;
			float cam_far_clip;
			float cam_fov;

			float cam_move_speed;
			float cam_rot_speed;

			bool gamma;

			// ao
			float ao_half_sample_radius;
			float ao_inv_distance_falloff;
			float ao_hieght_bias;
			float ao_hiz_mip_scaler;
			float ao_max_dist_sqr;

			//tonemap
			float tonemap_shoulder_strength;
			float tonemap_linear_strength;
			float tonemap_linear_angle;
			float tonemap_toe_strength;
			float tonemap_toe_numerator;
			float tonemap_toe_denominator;
			float tonemap_white_point;
			float tonemap_exposure_adjustment;
			bool tonemap_apply_to_lum;
			float tonemap_middle_gray;

			//hdr
			float hdr_bloom_amount;
			float hdr_bloom_mul;
			float hdr_bloom_threshold;
			float hdr_bloom_max;
			float hdr_adopt_speed;
			float hdr_limit_min;
			float hdr_limit_max;

			//smaa
			float smaa_threshold;
			float smaa_search_steps;
			float smaa_search_steps_diag;
			float smaa_corner_rounding;
		} EngSets;

	EngineSettings()
	{
		EngSets.fullscreen = false;
		EngSets.vsync = false;
		EngSets.win_border = true;
		EngSets.wres = 1280;
		EngSets.hres = 720;
		EngSets.fpslock = 60;
		EngSets.localization = L"EN";
		EngSets.msaa = false;
		EngSets.msaa_counts = 2;
		EngSets.msaa_quality = 4;
		EngSets.texfilter_quality = 2;
		EngSets.mipmap_offset = 0.0f;
		EngSets.hud_fps = false;
			
		EngSets.cam_near_clip = 0.01f;
		EngSets.cam_far_clip = 100000.0f;
		EngSets.cam_fov = XM_PIDIV2;

		EngSets.cam_move_speed = 0.02f;
		EngSets.cam_rot_speed = 0.01f;

		EngSets.gamma = false;

		EngSets.ao_half_sample_radius = 0.25f;
		EngSets.ao_inv_distance_falloff = 1.0f;
		EngSets.ao_hieght_bias = 0.05f;
		EngSets.ao_hiz_mip_scaler = 0.25f;
		EngSets.ao_max_dist_sqr = 256.0f;

		EngSets.tonemap_shoulder_strength = 0.22f;
		EngSets.tonemap_linear_strength = 0.3f;
		EngSets.tonemap_linear_angle = 0.1f;
		EngSets.tonemap_toe_strength = 0.2f;
		EngSets.tonemap_toe_numerator = 0.01f;
		EngSets.tonemap_toe_denominator = 0.3f;
		EngSets.tonemap_white_point = 4.0f;
		EngSets.tonemap_exposure_adjustment = 1.0f;
		EngSets.tonemap_apply_to_lum = false;
		EngSets.tonemap_middle_gray = 0.5f;

		EngSets.hdr_bloom_amount = 2.0f;
		EngSets.hdr_bloom_mul = 1.0f;
		EngSets.hdr_bloom_threshold = 1.5f;
		EngSets.hdr_bloom_max = 1.0f;
		EngSets.hdr_adopt_speed = 0.001f;
		EngSets.hdr_limit_min = 0.025f;
		EngSets.hdr_limit_max = 1.25f;

		EngSets.smaa_threshold = 1.25f;
		EngSets.smaa_search_steps = 32.0f;
		EngSets.smaa_search_steps_diag = 16.0f;
		EngSets.smaa_corner_rounding = 25.0f;

		ReadConfig(PATH_ENGINE_CONFIG);
	}

	private:
		void ReadConfig(string filename)
		{
			FileIO file(filename);
			auto root = file.Root();
			if(!root)
				return;
			auto cfg = file.Node(L"config", root);
			if(!cfg)
				return;

#define READCFG(name, type) EngSets.name = file.Read##type(L#name, cfg)

			READCFG(fullscreen, Bool);
			READCFG(vsync, Bool);
			READCFG(fpslock, Int);
			READCFG(wres, Int);
			READCFG(hres, Int);
			READCFG(localization, String);
			READCFG(msaa, Bool);
			READCFG(msaa_counts, Int);
			READCFG(msaa_quality, Int);
			READCFG(texfilter_quality, Int);
			READCFG(mipmap_offset, Float);
			READCFG(gamma, Bool);

			READCFG(cam_fov, Float);
			READCFG(cam_near_clip, Float);
			READCFG(cam_far_clip, Float);
			READCFG(cam_move_speed, Float);
			READCFG(cam_rot_speed, Float);

			READCFG(ao_half_sample_radius, Float);
			READCFG(ao_inv_distance_falloff, Float);
			READCFG(ao_hieght_bias, Float);
			READCFG(ao_hiz_mip_scaler, Float);
			READCFG(ao_max_dist_sqr, Float);

			READCFG(tonemap_shoulder_strength, Float);
			READCFG(tonemap_linear_strength, Float);
			READCFG(tonemap_linear_angle, Float);
			READCFG(tonemap_toe_strength, Float);
			READCFG(tonemap_toe_numerator, Float);
			READCFG(tonemap_toe_denominator, Float);
			READCFG(tonemap_white_point, Float);
			READCFG(tonemap_exposure_adjustment, Float);
			READCFG(tonemap_apply_to_lum, Bool);
			READCFG(tonemap_middle_gray, Float);

			READCFG(hdr_bloom_amount, Float);
			READCFG(hdr_bloom_mul, Float);
			READCFG(hdr_bloom_threshold, Float);
			READCFG(hdr_bloom_max, Float);
			READCFG(hdr_adopt_speed, Float);
			READCFG(hdr_limit_min, Float);
			READCFG(hdr_limit_max, Float);

			READCFG(smaa_threshold, Float);
			READCFG(smaa_search_steps, Float);
			READCFG(smaa_search_steps_diag, Float);
			READCFG(smaa_corner_rounding, Float);
		}
	};
}