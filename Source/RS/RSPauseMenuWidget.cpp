#include "RSPauseMenuWidget.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"

void URSPauseMenuWidget::FocusFirstButton()
{
	if (ResumeButton && GetOwningPlayer())
	{
		ResumeButton->SetUserFocus(GetOwningPlayer());
	}
}