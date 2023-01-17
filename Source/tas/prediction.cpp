#include "cpp_quakedef.hpp"
#include "hooks.h"
#include "draw.hpp"
#include "prediction.hpp"
#include "simulate.hpp"

using namespace TASQuake;

cvar_t tas_predict_per_frame{"tas_predict_per_frame", "0.01"};
cvar_t tas_predict{"tas_predict", "1"};
cvar_t tas_predict_grenade { "tas_predict_grenade", "0" };
cvar_t tas_predict_endoffset{"tas_predict_endoffset", "0.5"};
cvar_t tas_predict_maxlength{"tas_predict_maxlength", "10"};
const std::array<float, 4> color = { 0, 0, 1, 0.5 };
static std::vector<PathPoint> points;
static std::vector<Rect> rects;

bool path_assigned = false;
bool grenade_assigned = false;

bool Prediction_HasLine() {
	return path_assigned;
}

std::vector<PathPoint>* Prediction_GetPoints() {
	return &points;
}

std::vector<Rect>* Prediction_GetRects() {
	return &rects;
}

void TASQuake::Get_Prediction_Frames(int32_t& start_frame, int32_t& end_frame) {
    auto info = GetPlaybackInfo();
    start_frame = info->current_frame;
    end_frame = info->Get_Last_Frame() + tas_predict_endoffset.value * 72;
    int diffy = end_frame - start_frame;
    diffy = std::max(144, diffy);
    diffy = std::min<int32_t>(tas_predict_maxlength.value * 72, diffy);
    end_frame = start_frame + diffy;
}

bool Calculate_Prediction_Line(bool canPredict)
{
	static int startFrame = -1;
	if((!canPredict || tas_predict.value == 0)) {
		if(startFrame != -1) {
			startFrame = -1;
			RemoveCurve(PREDICTION_ID);
			RemoveRectangles(PREDICTION_ID);
			path_assigned = false;
		}
		return false;
	}

	auto playback = GetPlaybackInfo();
	static double last_updated = 0; 
	double currentTime = Sys_DoubleTime();
	static int32_t last_sim_frame = 0;
	static Simulator sim;

	if (last_updated < playback->last_edited || startFrame != playback->current_frame)
	{
		rects.clear();
		points.clear();
		last_updated = currentTime;
		startFrame = playback->current_frame;
		sim = Simulator::GetSimulator();
		int32_t start_frame;
		TASQuake::Get_Prediction_Frames(start_frame, last_sim_frame);

		points.reserve(last_sim_frame - start_frame);
	}

	double realTimeStart = Sys_DoubleTime();

	while (Sys_DoubleTime() - realTimeStart < tas_predict_per_frame.value && sim.frame < last_sim_frame)
	{
		PathPoint vec;
		vec.color[3] = 1;
		if (sim.info.collision)
		{
			vec.color[0] = 1;
		}
		else
		{
			vec.color[1] = 1;
		}
		VectorCopy(sim.info.ent.v.origin, vec.point);
		points.push_back(vec);

		auto block = playback->Get_Current_Block(sim.frame);
		if (block && block->frame == sim.frame)
		{
			Rect rect = Rect::Get_Rect(color, sim.info.ent.v.origin, 3, 3, PREDICTION_ID);
			rects.push_back(rect);
		}

		sim.RunFrame();
	}

	if (!path_assigned)
	{
		path_assigned = true;
	}

	return sim.frame >= last_sim_frame;
}

void Predict_Grenade(std::function<void(vec3_t)> frameCallback, std::function<void(vec3_t)> finalCallback, vec3_t origin, vec3_t v_angle)
{
	if(origin == NULL)
		origin = sv_player->v.origin;
	if(v_angle == NULL)
		v_angle = cl.viewangles;

	RNG rng;
	rng.SetSeed(Get_RNG_Seed());

	edict_t t;
	t.v.movetype = MOVETYPE_BOUNCE;
	t.v.solid = SOLID_BBOX;
	VectorCopy(vec3_origin, t.v.size);
	VectorCopy(vec3_origin, t.v.velocity);
	VectorCopy(origin, t.v.origin);
	SimulateSetMinMaxSize(&t, vec3_origin, vec3_origin);
	t.v.avelocity[0] = t.v.avelocity[1] = t.v.avelocity[2] = 300;

	vec3_t fwd, right, up;
	AngleVectors(v_angle, fwd, right, up);

	VectorScaledAdd(t.v.velocity, fwd, 600, t.v.velocity);
	VectorScaledAdd(t.v.velocity, right, 10 * rng.crandom(), t.v.velocity);
	VectorScaledAdd(t.v.velocity, up, 10 * rng.crandom(), t.v.velocity);
	VectorScaledAdd(t.v.velocity, up, 200, t.v.velocity);

	const float alivetime = 2.5f;
	double hfr = 1 / cl_maxfps.value;
	int frames = alivetime * cl_maxfps.value;

	for (int i = 0; i < frames; ++i)
	{
		Simulate_SV_Physics_Toss(&t, hfr);
		frameCallback(t.v.origin);
	}

	finalCallback(t.v.origin);
}

void Calculate_Grenade_Line(bool canPredict)
{
	if (!canPredict || tas_predict_grenade.value == 0)
	{
		if (grenade_assigned)
		{
			RemoveCurve(GRENADE_ID);
			grenade_assigned = false;
		}
		return;
	}

	static vec3_t grenade_start_origin;
	static vec3_t grenade_start_angles;
	static std::vector<PathPoint> points;
	const std::array<float, 4> color = { 1, 0, 1, 0.5 };
	auto checkvec = [](vec3_t first, vec3_t second) { return first[0] == second[0] && first[1] == second[1] && first[2] == second[2]; };
	
	if (grenade_assigned && checkvec(sv_player->v.origin, grenade_start_origin) && checkvec(cl.viewangles, grenade_start_angles))
	{
		return;
	}

	points.clear();
	points.reserve(static_cast<int>(2.5 * 72));
	VectorCopy(sv_player->v.origin, grenade_start_origin);
	VectorCopy(cl.viewangles, grenade_start_angles);

	auto addpoint = [&](vec3_t point) { PathPoint p; p.color = color; VectorCopy(point, p.point); points.push_back(p);};
	Predict_Grenade(addpoint, [](vec3_t){}, grenade_start_origin, grenade_start_angles);

	if (!grenade_assigned)
	{
		grenade_assigned = true;
		AddCurve(&points, GRENADE_ID);
	}
}
