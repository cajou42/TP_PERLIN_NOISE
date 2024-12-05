// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Noise.generated.h"

UCLASS()
class TP_PERLIN_NOISE_API ANoise : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ANoise();
	int GridSize = 500;
	int TotalOctave = 3;

	TArray<uint8_t> Pixels;
	TArray<int32> PermTable;
	UTexture2D* NoiseTexture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	UStaticMeshComponent* Mesh;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	float DotProduct(int ix, int iy, float x, float y);
	float Perlin(float x, float y);

	void WriteDataToTexture(UTexture2D* Texture, const TArray<uint8_t>& ColorData, int32 Width, int32 Height);

	float Fade(float t);
	float Lerp(float t, float a1, float a2);

	void GeneratePermutationTable(int32 Seed);
	FVector2d GetConstantVector(int32 v);

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
