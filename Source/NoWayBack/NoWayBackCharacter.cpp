// Copyright Epic Games, Inc. All Rights Reserved.

#include "NoWayBackCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ANoWayBackCharacter

void ANoWayBackCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	// {
	// 	FInputModeGameAndUI InputMode;
	// 	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture); 
	// 	PlayerController->SetInputMode(InputMode);
	// 	
	// 	// Show the mouse cursor
	// 	PlayerController->bShowMouseCursor = true;
	// }
}

ANoWayBackCharacter::ANoWayBackCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...	

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// // Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
	
	CameraBoom->bUsePawnControlRotation = false; // Rotate the arm based on the controller
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	
	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	PrimaryActorTick.bCanEverTick = true;
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANoWayBackCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ANoWayBackCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANoWayBackCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANoWayBackCharacter::Look);
		EnhancedInputComponent->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ANoWayBackCharacter::LookMouse);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ANoWayBackCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// In a top-down game, we want world-space directions
		// Forward in world space is +X
		const FVector ForwardDirection = FVector(1.0f, 0.0f, 0.0f);
		// Right in world space is +Y
		const FVector RightDirection = FVector(0.0f, 1.0f, 0.0f);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ANoWayBackCharacter::Look(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;
	
	// input is a Vector2D
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	UE_LOG(LogTemplateCharacter, Log, TEXT("'%s' - Look with gamepad x: %f, y: %f"), *GetNameSafe(this), LookAxisVector.X, LookAxisVector.Y);
	
	if (!FMath::IsNearlyZero(LookAxisVector.SizeSquared()))
	{
		// Convert the stick input to a world direction
		const FRotator InputRotation = FRotationMatrix::MakeFromX(
			FVector(-LookAxisVector.Y, LookAxisVector.X, 0.0f)
		).Rotator();
        
		// Set the character's rotation
		SetActorRotation(InputRotation);
	}
}
void ANoWayBackCharacter::LookMouse(const FInputActionValue& Value)
{
	if (Controller == nullptr) return;
	
	// input is a Vector2D
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	UE_LOG(LogTemplateCharacter, Log, TEXT("'%s' - Look with mouse x: %f, y: %f"), *GetNameSafe(this), LookAxisVector.X, LookAxisVector.Y);

	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (PlayerController == nullptr) return;

	// Get the mouse position in the world
	if (FVector WorldLocation, WorldDirection; PlayerController->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		// Calculate the intersection point with the ground (assuming Z = 0)
		const FVector PlaneIntersection = FMath::LinePlaneIntersection(WorldLocation, WorldLocation + WorldDirection * 10000.0f, FVector::ZeroVector, FVector::UpVector);

		// Calculate the direction from the character to the intersection point
		const FVector DirectionToLook = (PlaneIntersection - GetActorLocation()).GetSafeNormal();

		// Calculate the new rotation
		const FRotator NewRotation = UKismetMathLibrary::MakeRotFromX(DirectionToLook);
        
		// Set the character's rotation
		SetActorRotation(NewRotation);
	}
	
}	