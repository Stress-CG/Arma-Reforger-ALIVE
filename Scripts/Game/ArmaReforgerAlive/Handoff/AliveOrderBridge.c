class AliveOrderBridge : Managed
{
	void ApplyInitialTacticalState(AliveSpawnSpec spawnSpec, AlivePhysicalAgentComponent physicalAgent, ARAL_ForceProfile profile, ARAL_Order activeOrder)
	{
		if (!physicalAgent)
			return;

		physicalAgent.SetDesiredCount(spawnSpec.m_iDesiredCount);
		physicalAgent.ReportAliveCount(profile.m_iStrength);
		physicalAgent.ReportVehicleState(profile.m_bVehicleOperational, profile.m_fVehicleHealthNormalized, profile.m_fVehicleFuelNormalized);
		physicalAgent.SetOrderContext(spawnSpec.m_sOrderId, profile.m_sAnchorObjectiveId);

		// This is the seam where strategic intent is translated into concrete AI behavior.
		// The tactical controller hookup should remain local to the spawned hierarchy and not modify strategic replication.
		if (activeOrder)
			physicalAgent.SetOrderProgress(activeOrder.m_fProgressNormalized, activeOrder.m_vLastKnownPosition);
		else
			physicalAgent.SetOrderProgress(0, profile.m_vVirtualPosition);
	}

	void SyncActiveOrder(AlivePhysicalAgentComponent physicalAgent, ARAL_ForceProfile profile, ARAL_Order activeOrder)
	{
		if (!physicalAgent || !profile)
			return;

		string orderId = "";
		vector anchorPosition = profile.m_vVirtualPosition;
		float progressNormalized = 0;

		if (activeOrder)
		{
			orderId = activeOrder.m_sId;
			progressNormalized = activeOrder.m_fProgressNormalized;
			if (activeOrder.m_vLastKnownPosition != "0 0 0")
				anchorPosition = activeOrder.m_vLastKnownPosition;
		}

		physicalAgent.SetOrderContext(orderId, profile.m_sAnchorObjectiveId);
		physicalAgent.SetOrderProgress(progressNormalized, anchorPosition);
	}

	AliveOrderProgressSnapshot CaptureOrderProgress(AlivePhysicalAgentComponent physicalAgent, ARAL_Order activeOrder)
	{
		AliveOrderProgressSnapshot snapshot = new AliveOrderProgressSnapshot();
		if (!physicalAgent)
			return snapshot;

		if (activeOrder)
			snapshot.m_sOrderId = activeOrder.m_sId;

		snapshot.m_fCompletionNormalized = physicalAgent.GetOrderProgress();
		snapshot.m_vAnchorPosition = physicalAgent.GetOrderAnchorPosition();
		return snapshot;
	}

	void WriteBackOrderProgress(ARAL_Order activeOrder, AliveOrderProgressSnapshot snapshot)
	{
		if (!activeOrder || !snapshot)
			return;

		activeOrder.m_fProgressNormalized = snapshot.m_fCompletionNormalized;
		activeOrder.m_vLastKnownPosition = snapshot.m_vAnchorPosition;
	}
}
