
#include "Flare.h"
#include "../Game/FlareGame.h"
#include "../Data/FlareQuestCatalog.h"
#include "FlareQuestManager.h"

#define LOCTEXT_NAMESPACE "FlareQuestManager"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UFlareQuestManager::UFlareQuestManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}


/*----------------------------------------------------
	Save
----------------------------------------------------*/

void UFlareQuestManager::Load(const FFlareQuestSave& Data)
{
	Game = Cast<AFlareGame>(GetOuter());

	QuestData = Data;

	TArray<FName> CurrentQuestIdentifiers;
	for (int QuestProgressIndex = 0; QuestProgressIndex <Data.QuestProgresses.Num(); QuestProgressIndex++)
	{
		CurrentQuestIdentifiers.Add(Data.QuestProgresses[QuestProgressIndex].QuestIdentifier);
	}

	// Load quests
	for (int QuestIndex = 0; QuestIndex <Game->GetQuestCatalog()->Quests.Num(); QuestIndex++)
	{

		FFlareQuestDescription* QuestDescription = &Game->GetQuestCatalog()->Quests[QuestIndex];

		// Create the quest
		UFlareQuest* Quest = NewObject<UFlareQuest>(this, UFlareQuest::StaticClass());
		Quest->Load(QuestDescription);

		// Find quest status
		int QuestProgressIndex = CurrentQuestIdentifiers.IndexOfByKey(QuestDescription->Identifier);
		if (QuestProgressIndex != INDEX_NONE)
		{
			// Current quests
			AvailableQuests.Add(Quest);
			Quest->Restore(Data.QuestProgresses[QuestProgressIndex]);
			if (Data.SelectedQuest == QuestDescription->Identifier)
			{
				SelectedQuest = Quest;
			}
		}
		else if (Data.SuccessfulQuests.Contains(QuestDescription->Identifier))
		{
			OldQuests.Add(Quest);
			Quest->SetStatus(EFlareQuestStatus::SUCCESSFUL);
		}
		else if (Data.AbandonnedQuests.Contains(QuestDescription->Identifier))
		{
			OldQuests.Add(Quest);
			Quest->SetStatus(EFlareQuestStatus::ABANDONNED);
		}
		else if (Data.FailedQuests.Contains(QuestDescription->Identifier))
		{
			OldQuests.Add(Quest);
			Quest->SetStatus(EFlareQuestStatus::FAILED);
		}
		else
		{
			AvailableQuests.Add(Quest);
			Quest->SetStatus(EFlareQuestStatus::AVAILABLE);
		}

		LoadCallbacks(Quest);
	}
}


FFlareQuestSave* UFlareQuestManager::Save()
{
	QuestData.QuestProgresses.Empty();
	QuestData.SuccessfulQuests.Empty();
	QuestData.AbandonnedQuests.Empty();
	QuestData.FailedQuests.Empty();

	QuestData.SelectedQuest = (SelectedQuest ? SelectedQuest->GetIdentifier() : NAME_None);

	for (int QuestIndex = 0; QuestIndex < CurrentQuests.Num(); QuestIndex++)
	{
		UFlareQuest* Quest = CurrentQuests[QuestIndex];
		FFlareQuestProgressSave* QuestProgressSave = Quest->Save();
		QuestData.QuestProgresses.Add(*QuestProgressSave);
	}

	for (int QuestIndex = 0; QuestIndex < OldQuests.Num(); QuestIndex++)
	{
		UFlareQuest* Quest = OldQuests[QuestIndex];
		switch (Quest->GetStatus())
		{
			case EFlareQuestStatus::SUCCESSFUL:
				QuestData.SuccessfulQuests.Add(Quest->GetIdentifier());
				break;
			case EFlareQuestStatus::ABANDONNED:
				QuestData.AbandonnedQuests.Add(Quest->GetIdentifier());
				break;
			case EFlareQuestStatus::FAILED:
				QuestData.FailedQuests.Add(Quest->GetIdentifier());
				break;
			default:
				FLOGV("Bad status %d for quest %s", (int) (Quest->GetStatus() + 0), *Quest->GetIdentifier().ToString());
		}
	}

	return &QuestData;
}

/*----------------------------------------------------
	Callback
----------------------------------------------------*/

void UFlareQuestManager::LoadCallbacks(UFlareQuest* Quest)
{
	ClearCallbacks(Quest);

	TArray<EFlareQuestCallback::Type> Callbacks = Quest->GetCurrentCallbacks();

	for (int i = 0; i < Callbacks.Num(); i++)
	{
		switch(Callbacks[i])
		{
			case EFlareQuestCallback::FLY_SHIP:
				FlyShipCallback.Add(Quest);
				break;
			case EFlareQuestCallback::TICK:
				TickCallback.Add(Quest);
				break;
			default:
				FLOGV("Bad callback type %d for quest %s", (int) (Callbacks[i] + 0), *Quest->GetIdentifier().ToString());
		}
	}
}

void UFlareQuestManager::ClearCallbacks(UFlareQuest* Quest)
{
	TickCallback.Remove(Quest);
	FlyShipCallback.Remove(Quest);
}

void UFlareQuestManager::OnTick(float DeltaSeconds)
{
	for (int i = 0; i < TickCallback.Num(); i++)
	{
		TickCallback[i]->OnTick(DeltaSeconds);
	}
}

void UFlareQuestManager::OnFlyShip(AFlareSpacecraft* Ship)
{
	for (int i = 0; i < FlyShipCallback.Num(); i++)
	{
		FlyShipCallback[i]->OnFlyShip(Ship);
	}
}


#undef LOCTEXT_NAMESPACE
