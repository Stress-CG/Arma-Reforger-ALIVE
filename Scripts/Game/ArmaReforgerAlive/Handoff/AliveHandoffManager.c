class AliveHandoffManager : Managed
{
	protected ref AliveProfileManager m_ProfileManager = new AliveProfileManager();
	protected ref AliveOrderBridge m_OrderBridge = new AliveOrderBridge();
	protected ref AliveTacticalControllerService m_TacticalControllerService = new AliveTacticalControllerService();

	void ResetRuntime(bool deletePhysicalProjections = true)
	{
		if (deletePhysicalProjections)
			ReleaseAllPhysicalProjections();

		m_ProfileManager.Reset();
	}

	void Tick(IEntity owner, ARAL_WarState warState, ARAL_MissionPreset preset, array<vector> playerObservers)
	{
		if (!owner || !warState || !preset)
			return;

		if (!Replication.IsServer())
			return;

		IAliveSpawnCatalog spawnCatalog = ResolveSpawnCatalog(owner);
		float now = System.GetUnixTime();

		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			AliveProfileRuntimeRecord runtimeRecord = m_ProfileManager.EnsureRuntimeRecord(profile, now);
			m_ProfileManager.RefreshRuntime(profile, runtimeRecord, now);
			AliveRelevanceContext relevance = m_ProfileManager.BuildRelevance(profile, preset, runtimeRecord, playerObservers, now);
			ARAL_Order activeOrder = ARAL_WarStateUtility.FindOrderById(warState, profile.m_sActiveOrderId);
			AliveProfileBinding binding = m_ProfileManager.FindBindingByProfileId(profile.m_sId);
			AlivePhysicalAgentComponent physicalAgent;
			IEntity boundEntity;
			if (binding)
			{
				boundEntity = binding.m_Entity;
				if (boundEntity)
					physicalAgent = AlivePhysicalAgentComponent.Cast(boundEntity.FindComponent(AlivePhysicalAgentComponent));
			}

			if (binding && !boundEntity)
			{
				HandleProjectionLoss(profile, runtimeRecord, now);
				continue;
			}

			if (boundEntity)
			{
				if (relevance.m_bStillRelevant)
				{
					if (physicalAgent)
						physicalAgent.MarkObserved(now);
					m_ProfileManager.MarkObserved(runtimeRecord, now);
				}

				if (physicalAgent)
					m_OrderBridge.SyncActiveOrder(physicalAgent, profile, activeOrder);

				AliveTacticalUpdateSnapshot tacticalUpdate = m_TacticalControllerService.UpdateProjection(warState, profile, activeOrder, boundEntity, physicalAgent, playerObservers, now);
				m_ProfileManager.SetTacticalRuntimeState(runtimeRecord, tacticalUpdate);
				m_OrderBridge.WriteBackOrderProgress(activeOrder, tacticalUpdate.m_OrderProgress);
				if (m_TacticalControllerService.IsProjectionEngaged(warState, activeOrder, boundEntity) || tacticalUpdate.m_bFiredRecently || tacticalUpdate.m_bTookDamageRecently || tacticalUpdate.m_bHasEnemyTarget)
				{
					m_ProfileManager.MarkEngaged(runtimeRecord, now);
					if (physicalAgent)
						physicalAgent.MarkEngaged(now);
				}

				if (ShouldReclaimProfile(profile, runtimeRecord, relevance, physicalAgent, now))
					BeginReclaimTransaction(profile, runtimeRecord, activeOrder, boundEntity, physicalAgent, now);

				continue;
			}

			if (ShouldSpawnProfile(profile, runtimeRecord, relevance, now))
				BeginSpawnTransaction(owner, profile, runtimeRecord, activeOrder, spawnCatalog, now);
		}
	}

	protected bool ShouldSpawnProfile(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, AliveRelevanceContext relevance, float now)
	{
		if (profile.m_iStrength <= 0)
			return false;

		if (profile.m_eState != ARAL_EProfileState.MATERIALIZED)
			return false;

		if (!relevance.CanSpawn())
			return false;

		return m_ProfileManager.TryReserveForSpawn(profile, runtimeRecord, now);
	}

	protected bool ShouldReclaimProfile(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, AliveRelevanceContext relevance, AlivePhysicalAgentComponent physicalAgent, float now)
	{
		if (profile.m_eState != ARAL_EProfileState.VIRTUAL)
		{
			m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "profile-still-materialized");
			return false;
		}

		if (!m_ProfileManager.CanBeginReclaim(runtimeRecord, relevance, now))
		{
			if (relevance.m_bStillRelevant)
				m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "player-nearby");
			else if (relevance.m_bRecentlyVisible)
				m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "recent-visibility");
			else if (relevance.m_bRecentlyEngaged)
				m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "recent-engagement");
			else
				m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "cooldown");
			return false;
		}

		if (physicalAgent && !physicalAgent.CanBeReclaimed(now, 45.0, 12.0))
		{
			if (physicalAgent.IsPlayerOccupancyBlocked())
				m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "player-in-vehicle");
			else if (physicalAgent.IsVehicleMoving())
				m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "vehicle-moving");
			else
				m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "physical-visibility-grace");
			return false;
		}

		m_ProfileManager.SetReclaimBlockReason(runtimeRecord, "");
		return true;
	}

	protected bool BeginSpawnTransaction(IEntity owner, ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, ARAL_Order activeOrder, IAliveSpawnCatalog spawnCatalog, float now)
	{
		AliveSpawnSpec spawnSpec = m_ProfileManager.BuildSpawnSpec(profile, activeOrder, runtimeRecord);
		ResourceName resolvedRootPrefab = "";
		if (!spawnCatalog || !spawnCatalog.ResolveRootPrefab(profile.m_sTemplateId, resolvedRootPrefab))
		{
			ARAL_Log.Warning(string.Format("No spawn prefab configured for profile template '%1'.", profile.m_sTemplateId));
			m_ProfileManager.RollbackSpawnReservation(runtimeRecord, now);
			return false;
		}
		spawnSpec.m_sRootPrefab = resolvedRootPrefab;

		m_ProfileManager.MarkSpawnStarted(runtimeRecord, now);

		IEntity spawnedRoot = SpawnPhysicalProjection(owner, spawnSpec);
		if (!spawnedRoot)
		{
			m_ProfileManager.RollbackSpawnReservation(runtimeRecord, now);
			return false;
		}

		AlivePhysicalAgentComponent physicalAgent = AlivePhysicalAgentComponent.Cast(spawnedRoot.FindComponent(AlivePhysicalAgentComponent));
		bool rawProjectionFallback = false;
		if (physicalAgent)
		{
			physicalAgent.BindToProfile(profile.m_sId, profile.m_sTemplateId, runtimeRecord.m_iSpawnGeneration, profile.m_iStrength);
			m_OrderBridge.ApplyInitialTacticalState(spawnSpec, physicalAgent, profile, activeOrder);
		}
		else
		{
			rawProjectionFallback = true;
			ARAL_Log.Warning(string.Format("Projection '%1' spawned without AlivePhysicalAgentComponent. Using raw fallback binding.", profile.m_sId));
		}

		AliveProfileBinding binding = m_ProfileManager.CreateBinding(profile.m_sId, spawnedRoot, runtimeRecord.m_iSpawnGeneration, now, rawProjectionFallback);
		m_ProfileManager.CompleteSpawn(profile, runtimeRecord, binding, now);
		ARAL_Log.Info(string.Format("Spawn binding established %1 -> %2", profile.m_sId, FormatEntityId(binding.m_EntityId)));
		return true;
	}

	protected bool BeginReclaimTransaction(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, ARAL_Order activeOrder, IEntity boundEntity, AlivePhysicalAgentComponent physicalAgent, float now)
	{
		AliveProfileBinding binding = m_ProfileManager.FindBindingByProfileId(profile.m_sId);
		string entityId = binding ? FormatEntityId(binding.m_EntityId) : (boundEntity ? FormatEntityId(boundEntity.GetID()) : "-");

		m_ProfileManager.BeginReclaim(runtimeRecord, now);
		if (physicalAgent)
			physicalAgent.PrepareForReclaim();
		m_ProfileManager.MarkReclaiming(runtimeRecord, now);

		AlivePhysicalSnapshot snapshot = CaptureProjectionSnapshot(profile, activeOrder, boundEntity, physicalAgent, now);
		m_OrderBridge.WriteBackOrderProgress(activeOrder, snapshot.m_OrderProgress);
		m_ProfileManager.WriteBackSnapshot(profile, activeOrder, snapshot);

		if (boundEntity)
			DeleteProjectionEntity(boundEntity);

		m_ProfileManager.RemoveBinding(profile.m_sId);
		if (profile.m_iStrength <= 0)
			m_ProfileManager.MarkDestroyed(profile, runtimeRecord, now);
		else
			m_ProfileManager.CompleteReclaim(profile, runtimeRecord, now);

		ARAL_Log.Info(string.Format("Reclaim binding completed %1 -> %2", entityId, profile.m_sId));

		return true;
	}

	protected IEntity SpawnPhysicalProjection(IEntity owner, AliveSpawnSpec spawnSpec)
	{
		Resource prefab = Resource.Load(spawnSpec.m_sRootPrefab);
		if (!prefab || !prefab.IsValid())
			return null;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		spawnParams.TransformMode = ETransformMode.WORLD;
		for (int i = 0; i < 4; i++)
			spawnParams.Transform[i] = spawnSpec.m_mTransform[i];

		if (!ChimeraGame.CanSpawnEntityPrefab(prefab, spawnParams))
			return null;

		ArmaReforgerScripted game = ArmaReforgerScripted.Cast(GetGame());
		if (!game)
			return null;

		IEntity entity = game.SpawnEntityPrefabEx(spawnSpec.m_sRootPrefab, false, owner.GetWorld(), spawnParams);
		if (!entity)
			return null;

		RplComponent rplComponent = SCR_EntityHelper.GetEntityRplComponent(entity);
		if (!rplComponent)
		{
			ARAL_Log.Warning(string.Format("Spawned projection '%1' does not have a root RplComponent and cannot be used for multiplayer-safe handoff.", spawnSpec.m_sProfileId));
			SCR_EntityHelper.DeleteEntityAndChildren(entity);
			return null;
		}

		if (rplComponent && !rplComponent.IsSelfInserted())
			rplComponent.InsertToReplication();
		if (rplComponent)
		{
			rplComponent.EnableSpatialRelevancy(true);
			rplComponent.EnableStreaming(true);
		}

		return entity;
	}

	bool HasPhysicalProjection(string profileId)
	{
		AliveProfileBinding binding = m_ProfileManager.FindBindingByProfileId(profileId);
		return binding && binding.m_Entity;
	}

	int GetPhysicalProjectionCount()
	{
		return m_ProfileManager.GetBindingCount();
	}

	int GetRawProjectionCount()
	{
		return m_ProfileManager.CountRawBindings();
	}

	int GetPendingProjectionCount()
	{
		return m_ProfileManager.CountProfilesInState(AliveEHandoffState.PendingSpawn) + m_ProfileManager.CountProfilesInState(AliveEHandoffState.Spawning);
	}

	string BuildDebugSummary(ARAL_WarState warState)
	{
		return string.Format(
			"%1 | physical=%2 | raw=%3 | pendingSpawn=%4 | spawning=%5 | pendingReclaim=%6 | reclaiming=%7 | destroyed=%8",
			ARAL_ProjectPolicy.BuildPolicySummary(),
			GetPhysicalProjectionCount(),
			GetRawProjectionCount(),
			m_ProfileManager.CountProfilesInState(AliveEHandoffState.PendingSpawn),
			m_ProfileManager.CountProfilesInState(AliveEHandoffState.Spawning),
			m_ProfileManager.CountProfilesInState(AliveEHandoffState.PendingReclaim),
			m_ProfileManager.CountProfilesInState(AliveEHandoffState.Reclaiming),
			m_ProfileManager.CountProfilesInState(AliveEHandoffState.Destroyed)
		);
	}

	string BuildAdminProfileOverlay(ARAL_WarState warState)
	{
		if (!warState || warState.m_aProfiles.IsEmpty())
			return "No profiles";

		string overlay = "";
		int limit = Math.Min(warState.m_aProfiles.Count(), 8);
		for (int i = 0; i < limit; i++)
		{
			ARAL_ForceProfile profile = warState.m_aProfiles[i];
			AliveProfileRuntimeRecord runtimeRecord = m_ProfileManager.FindRuntimeRecord(profile.m_sId);
			AliveProfileBinding binding = m_ProfileManager.FindBindingByProfileId(profile.m_sId);
			ARAL_Order activeOrder = ARAL_WarStateUtility.FindOrderById(warState, profile.m_sActiveOrderId);

			string line = string.Format(
				"%1 state=%2 entity=%3 pos=%4 order=%5 mode=%6",
				profile.m_sId,
				runtimeRecord ? ARAL_EnumUtility.HandoffStateToString(runtimeRecord.m_eHandoffState) : "Virtual",
				binding ? FormatEntityId(binding.m_EntityId) : "-",
				FormatVectorCompact(profile.m_vVirtualPosition),
				activeOrder ? ARAL_EnumUtility.OrderTypeToString(activeOrder.m_eType) : "None",
				runtimeRecord ? ARAL_EnumUtility.TacticalModeToString(runtimeRecord.m_eLastTacticalMode) : "None"
			);

			if (overlay != "")
				overlay += "\n";
			overlay += line;
		}

		return overlay;
	}

	string BuildAdminReclaimOverlay(ARAL_WarState warState)
	{
		if (!warState || warState.m_aProfiles.IsEmpty())
			return "No reclaim blockers";

		string overlay = "";
		int added = 0;
		foreach (ARAL_ForceProfile profile : warState.m_aProfiles)
		{
			AliveProfileRuntimeRecord runtimeRecord = m_ProfileManager.FindRuntimeRecord(profile.m_sId);
			if (!runtimeRecord || runtimeRecord.m_sLastReclaimBlockReason == "")
				continue;

			if (overlay != "")
				overlay += "\n";

			overlay += string.Format("%1 block=%2 speed=%3", profile.m_sId, runtimeRecord.m_sLastReclaimBlockReason, Math.Round(runtimeRecord.m_fEstimatedSpeedMps * 10.0) / 10.0);
			added++;
			if (added >= 8)
				break;
		}

		if (overlay == "")
			return "No reclaim blockers";

		return overlay;
	}

	protected void ReleaseAllPhysicalProjections()
	{
		array<ref AliveProfileBinding> bindings = {};
		m_ProfileManager.GetBindingsSnapshot(bindings);
		foreach (AliveProfileBinding binding : bindings)
		{
			if (binding && binding.m_Entity)
				DeleteProjectionEntity(binding.m_Entity);
		}
	}

	protected IAliveSpawnCatalog ResolveSpawnCatalog(IEntity owner)
	{
		if (!owner)
			return null;

		return AliveSpawnCatalogComponent.Cast(owner.FindComponent(AliveSpawnCatalogComponent));
	}

	protected void HandleProjectionLoss(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, float now)
	{
		ARAL_Log.Warning(string.Format("Physical projection for profile '%1' was lost. Returning to virtual authority state.", profile.m_sId));
		m_ProfileManager.RemoveBinding(profile.m_sId);
		if (profile.m_iStrength <= 0)
			m_ProfileManager.MarkDestroyed(profile, runtimeRecord, now);
		else
			m_ProfileManager.RecoverLostProjection(profile, runtimeRecord, now);
	}

	protected AlivePhysicalSnapshot CaptureProjectionSnapshot(ARAL_ForceProfile profile, ARAL_Order activeOrder, IEntity boundEntity, AlivePhysicalAgentComponent physicalAgent, float now)
	{
		if (physicalAgent)
			return physicalAgent.CaptureSnapshot(boundEntity);

		AlivePhysicalSnapshot snapshot = new AlivePhysicalSnapshot();
		snapshot.m_sProfileId = profile.m_sId;
		snapshot.m_fCapturedAt = now;
		snapshot.m_iAliveCount = profile.m_iStrength;
		snapshot.m_vPosition = profile.m_vVirtualPosition;
		if (boundEntity)
			snapshot.m_vPosition = boundEntity.GetOrigin();

		snapshot.m_VehicleState.m_bHasVehicle = profile.m_bVehicleProfile;
		snapshot.m_VehicleState.m_bOperational = profile.m_bVehicleOperational;
		snapshot.m_VehicleState.m_fHealthNormalized = profile.m_fVehicleHealthNormalized;
		snapshot.m_VehicleState.m_fFuelNormalized = profile.m_fVehicleFuelNormalized;

		if (activeOrder)
		{
			snapshot.m_OrderProgress.m_sOrderId = activeOrder.m_sId;
			snapshot.m_OrderProgress.m_fCompletionNormalized = activeOrder.m_fProgressNormalized;
			snapshot.m_OrderProgress.m_vAnchorPosition = activeOrder.m_vLastKnownPosition;
		}

		return snapshot;
	}

	ARAL_WarState BuildPersistenceSnapshot(ARAL_WarState warState)
	{
		if (!warState)
			return null;

		float now = System.GetUnixTime();
		ARAL_WarState snapshotState = warState.Clone();
		snapshotState.EnsureCollections();
		snapshotState.m_aMaterializationRequests.Clear();

		foreach (ARAL_ForceProfile snapshotProfileDefault : snapshotState.m_aProfiles)
		{
			snapshotProfileDefault.m_iMaterializedCount = 0;
			snapshotProfileDefault.m_eState = ARAL_EProfileState.VIRTUAL;
		}

		array<ref AliveProfileBinding> bindings = {};
		m_ProfileManager.GetBindingsSnapshot(bindings);
		foreach (AliveProfileBinding binding : bindings)
		{
			if (!binding || !binding.m_Entity)
				continue;

			ARAL_ForceProfile liveProfile = ARAL_WarStateUtility.FindProfileById(warState, binding.m_sProfileId);
			ARAL_ForceProfile snapshotProfile = ARAL_WarStateUtility.FindProfileById(snapshotState, binding.m_sProfileId);
			if (!liveProfile || !snapshotProfile)
				continue;

			ARAL_Order liveOrder = ARAL_WarStateUtility.FindOrderById(warState, liveProfile.m_sActiveOrderId);
			ARAL_Order snapshotOrder = ARAL_WarStateUtility.FindOrderById(snapshotState, snapshotProfile.m_sActiveOrderId);
			AlivePhysicalAgentComponent physicalAgent = AlivePhysicalAgentComponent.Cast(binding.m_Entity.FindComponent(AlivePhysicalAgentComponent));
			AlivePhysicalSnapshot projectionSnapshot = CaptureProjectionSnapshot(liveProfile, liveOrder, binding.m_Entity, physicalAgent, now);
			m_ProfileManager.WriteBackSnapshot(snapshotProfile, snapshotOrder, projectionSnapshot);
			snapshotProfile.m_eState = ARAL_EProfileState.VIRTUAL;
		}

		return snapshotState;
	}

	protected void DeleteProjectionEntity(IEntity entity)
	{
		if (!entity)
			return;

		RplComponent rplComponent = SCR_EntityHelper.GetEntityRplComponent(entity);
		if (rplComponent)
		{
			RplComponent.DeleteRplEntity(entity, false);
			return;
		}

		SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}

	protected string FormatEntityId(EntityID entityId)
	{
		return string.Format("%1", entityId);
	}

	protected string FormatVectorCompact(vector position)
	{
		return string.Format("%1,%2", Math.Round(position[0]), Math.Round(position[2]));
	}
}
