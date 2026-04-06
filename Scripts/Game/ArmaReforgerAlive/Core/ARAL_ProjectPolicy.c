class ARAL_ProjectPolicy
{
	// V1 architecture decisions locked from the implementation checklist.
	static const bool USE_HYBRID_PREFAB_STRATEGY = true;
	static const bool ALLOW_RAW_VANILLA_FALLBACK = true;
	static const bool ADMIN_OVERLAY_ONLY = true;
	static const bool USE_LOCAL_JSON_PERSISTENCE_ONLY = true;
	static const bool EVERON_VALIDATION_ONLY = true;
	static const string VALIDATION_WORLD_NAME = "Everon";

	static string BuildPolicySummary()
	{
		return "policy=hybrid-prefab|admin-overlay|local-json|everon-only";
	}
}
