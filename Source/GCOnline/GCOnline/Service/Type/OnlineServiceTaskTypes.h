// Copyright (C) 2024 owoDra

#pragma once

//#include "OnlineServiceTaskTypes.generated.h"


/** 
 * Used to track the progress of different asynchronous operations 
 */
enum class EOnlineServiceTaskState : uint8
{
	// The task has not been started
	NotStarted,

	// The task is currently being processed
	InProgress,

	// The task has completed successfully
	Done,

	// The task failed to complete
	Failed
};
