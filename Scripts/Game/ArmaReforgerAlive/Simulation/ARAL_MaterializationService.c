class ARAL_MaterializationService : Managed
{
	void ReconcileRequests(ARAL_WarState warState, AliveHandoffManager handoffManager = null)
	{
		if (!warState)
			return;

		warState.EnsureCollections();

		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			ARAL_MaterializationRequest request = ARAL_WarStateUtility.FindMaterializationRequestByProfileId(warState, profile.m_sId);
			if (profile.m_eState == ARAL_EProfileState.MATERIALIZED)
			{
				if (!request)
				{
					request = new ARAL_MaterializationRequest();
					request.m_sProfileId = profile.m_sId;
					warState.m_aMaterializationRequests.Insert(request);
				}

				request.m_sFactionKey = profile.m_sFactionKey;
				request.m_sTemplateId = profile.m_sTemplateId;
				request.m_sDisplayName = profile.m_sDisplayName;
				request.m_vSpawnPosition = profile.m_vVirtualPosition;
				request.m_iDesiredCount = profile.m_iStrength;
				request.m_bVehicleProfile = profile.m_bVehicleProfile;
				request.m_eState = ARAL_EMaterializationState.PENDING;
				if (handoffManager && handoffManager.HasPhysicalProjection(profile.m_sId))
				{
					request.m_iDesiredCount = profile.m_iMaterializedCount;
					request.m_eState = ARAL_EMaterializationState.READY;
				}

				continue;
			}

			if (!request)
				continue;

			request.m_vSpawnPosition = profile.m_vVirtualPosition;
			request.m_iDesiredCount = 0;
			request.m_eState = ARAL_EMaterializationState.VIRTUALIZED;
		}
	}
}
