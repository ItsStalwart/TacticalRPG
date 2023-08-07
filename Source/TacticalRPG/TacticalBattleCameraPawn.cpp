// Fill out your copyright notice in the Description page of Project Settings.


#include "TacticalBattleCameraPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"


// Sets default values
ATacticalBattleCameraPawn::ATacticalBattleCameraPawn()
{
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	DummyComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DummyComponent"));
	DummyComponent->SetupAttachment(RootComponent);
	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(DummyComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // we need custom rotation because of gravity shifts

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovementComponent"));
}

// Called when the game starts or when spawned
void ATacticalBattleCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	DesiredRotation = GetActorRotation();
}


// Called every frame
void ATacticalBattleCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), DesiredRotation, DeltaTime, 5.f));
}

// Called to bind functionality to input
void ATacticalBattleCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		//Rotating
		EnhancedInputComponent->BindAction(RotateAction, ETriggerEvent::Started, this, &ATacticalBattleCameraPawn::RotateCamera);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATacticalBattleCameraPawn::MoveCamera);

		//Zooming
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ATacticalBattleCameraPawn::ZoomCamera);

		//Selecting Tiles
		EnhancedInputComponent->BindAction(SelectAction,ETriggerEvent::Triggered, this, &ATacticalBattleCameraPawn::TrySelectTile);

	}
}

void ATacticalBattleCameraPawn::RotateCamera(const FInputActionValue& InputActionValue)
{
	const bool RotateRight = InputActionValue.Get<float>() > 0 ;
	const FRotator RotateTo = RotateRight ? FRotator(0, 45.0, 0) : FRotator(0, -45.0, 0);
	DesiredRotation = UKismetMathLibrary::ComposeRotators(GetActorRotation(), RotateTo);
}

void ATacticalBattleCameraPawn::MoveCamera(const FInputActionValue& InputActionValue)
{
	const FVector2d MovementDirection = InputActionValue.Get<FVector2d>();
	// get forward vector
	const FVector ForwardDirection = GetActorForwardVector();
	
	// get right vector 
	const FVector RightDirection = GetActorRightVector();

	// add movement
	AddMovementInput(ForwardDirection, MovementDirection.Y * CameraMoveSpeed);
	AddMovementInput(RightDirection, MovementDirection.X * CameraMoveSpeed);
}

void ATacticalBattleCameraPawn::ZoomCamera(const FInputActionValue& InputActionValue)
{
	const float ZoomDirection = InputActionValue.Get<float>();
	CameraBoom-> TargetArmLength = FMath::Clamp(CameraBoom->TargetArmLength + ZoomDirection * CameraZoomSpeed, CameraMinZoomDistance, CameraMaxZoomDistance);
}

void ATacticalBattleCameraPawn::TrySelectTile(const FInputActionValue& InputActionValue)
{
	TriedSelectingTileEvent.Broadcast();
}

