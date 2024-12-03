// Fill out your copyright notice in the Description page of Project Settings.


#include "Noise.h"
#include <AssetRegistry/AssetRegistryModule.h>
#include <math.h>


// Sets default values
ANoise::ANoise()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh>MeshAsset(TEXT("/Script/Engine.StaticMesh'/Game/LevelPrototyping/Meshes/SM_Cube1.SM_Cube1'"));
	UStaticMesh* Asset = MeshAsset.Object;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("mesh"));
	Mesh->SetStaticMesh(Asset);

	RootComponent = Mesh;

	GeneratePermutationTable(FMath::Rand32());

}

// Create the Permatation table with a given seed
void ANoise::GeneratePermutationTable(int32 Seed)
{

	PermTable.Reserve(2048);

	// Initialize with sequential values
	for (int32 i = 0; i < 256; ++i)
	{
		PermTable.Add(i);
	}

	// Shuffle the table using a deterministic seed-based method
	FRandomStream RandomStream(Seed);
	for (int32 z = 255; z > 0; --z)
	{
		int32 J = RandomStream.RandRange(0, z);

		Swap(PermTable[z], PermTable[J]);
	}

	TArray<int32> Temp;
	Temp.Reserve(256);

	// Double the table for easy indexing
	for (int32 y = 0; y < 256; ++y)
	{
		Temp.Add(PermTable[y]); // Copy into temporary array
	}

	// Append all elements from Temp to PermTable
	PermTable.Append(Temp);
}

// Called when the game starts or when spawned
void ANoise::BeginPlay()
{
	Super::BeginPlay();

	FString AssetPath = FPaths::ProjectContentDir();
	FString AssetName = TEXT("NoiseTexture");
	FString PackagePath = TEXT("/Game/LevelPrototyping/Textures/") + AssetName;

	UPackage* Package = CreatePackage(*PackagePath);
	NoiseTexture = NewObject<UTexture2D>(Package, FName(AssetName), RF_Public | RF_Standalone);
	NoiseTexture->Source.Init(GridSize, GridSize, 1, 1, TSF_BGRA8);

	for(float x = 0.0f; x < GridSize; ++x)
	{
		for(float y = 0.0f; y < GridSize; ++y)
		{
			float PerlinValue = 0;
			float Frequency = 0.005f;
			float Amplitude = 1.0f;

			// Octave supperposition
			for (int NbOctave = 0; NbOctave < TotalOctave; NbOctave++)
			{
				PerlinValue += Perlin(x * Frequency, y * Frequency) * Amplitude;

				Frequency *= 2;
				Amplitude *= 0.5;
			}

			PerlinValue *= 1.2;

			// If the perlin value is not between 1 and -1
			if (PerlinValue > 1.0f)
				PerlinValue = 1.0f;
			else if (PerlinValue < -1.0f)
				PerlinValue = -1.0f;

			// Convert noise result into color value
			uint8_t Color = (uint8_t)(((PerlinValue + 1.0f) * 0.5f) * 255);

			// Set pixels values
			Pixels.Add(Color);
			Pixels.Add(Color);
			Pixels.Add(Color);
			Pixels.Add(255);
		}
	}

	WriteDataToTexture(NoiseTexture, Pixels, GridSize, GridSize);
	
	FAssetRegistryModule::AssetCreated(NoiseTexture);
	NoiseTexture->MarkPackageDirty();

	FString FilePath = FString::Printf(TEXT("%s%s%s"), *AssetPath, *AssetName, *FPackageName::GetAssetPackageExtension());
	bool IsSuccess = UPackage::SavePackage(Package, NoiseTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *FilePath);

}

// Make a dot product between a distance vector and a normalize gradient vector
float ANoise::DotProduct(int ix, int iy, float x, float y)
{
	// Generate gradient vecteur with the grid point
	int32 GradientVector = PermTable[(PermTable[ix] + iy)];

	// Calculate points for the distance vector
	FVector2d DistanceVector = FVector2d(x - (float)ix, y - (float)iy);

	return DistanceVector.Dot(GetConstantVector(GradientVector));
}

// Get Normalize vector value with a given value from the permutation table
FVector2D ANoise::GetConstantVector(int32 v)
{
	// v is the value from the permutation table
	const int32 h = v & 3;
	if (h == 0)
		return FVector2D(1.0, 1.0);
	if (h == 1)
		return FVector2D(-1.0, 1.0);
	if (h == 2)
		return FVector2D(-1.0, -1.0);
	return FVector2D(1.0, -1.0);
}

float ANoise::Perlin(float x, float y)
{
	// Upper left and Bottom left corners of the cell
	int Lx = (int)x;
	int Ly = (int)y;

	// Upper right and Bottom right corners of the cell
	int Rx = Lx + 1;
	int Ry = Ly + 1;

	// Interpolation weights
	float Wx = Fade(x - (float)Lx);
	float Wy = Fade(y - (float)Ly);

	// Dot Top corners
	float Dot1 = DotProduct(Lx, Ly, x, y);
	float Dot2 = DotProduct(Rx, Ly, x, y);

	// Interpolation for smooth perlin noise image
	float TopInterp = Lerp(Wx, Dot1, Dot2);

	// Dot Bottom corners
	Dot1 = DotProduct(Lx, Ry, x, y);
	Dot2 = DotProduct(Rx, Ry, x, y);

	// Interpolation for smooth perlin noise image
	float BottomInterp = Lerp(Wx, Dot1, Dot2);

	return Lerp(Wy, TopInterp, BottomInterp);
}

// Called every frame
void ANoise::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Fonction that write a set of data into a 2D texture
void ANoise::WriteDataToTexture(UTexture2D* Texture, const TArray<uint8_t>& ColorData, int32 Width, int32 Height)
{
	// Check if param are not wrong
	if (!Texture || ColorData.Num() != Width * Height * 4)
	{
		return;
	}

	// Lock the texture so we can modify it
	uint8_t* Data = Texture->Source.LockMip(0);

	// Copy color data
	FMemory::Memcpy(Data, ColorData.GetData(), ColorData.Num());

	// Unlock and update
	Texture->Source.UnlockMip(0);

	// Update texture properties
	Texture->SRGB = true; // Use sRGB color space for textures viewed directly on screen
	Texture->CompressionSettings = TC_Grayscale; // Use default compression for regular textures
	Texture->PostEditChange(); // Apply changes to the asset
	Texture->UpdateResource();
}

// Fonction that save a 2D Texture in the engine content drawer
void ANoise::SaveTexture()
{
	// Defining path
	FString AssetPath = FPaths::ProjectContentDir();
	FString AssetName = TEXT("NoiseTexture");
	FString PackagePath = TEXT("/Game/LevelPrototyping/Textures/") + AssetName;

	// Create package an assign texture to it
	UPackage* Package = CreatePackage(*PackagePath);
	FAssetRegistryModule::AssetCreated(NoiseTexture);
	NoiseTexture->MarkPackageDirty();

	// Save Package
	FString FilePath = FString::Printf(TEXT("%s%s%s"), *AssetPath, *AssetName, *FPackageName::GetAssetPackageExtension());
	bool IsSuccess = UPackage::SavePackage(Package, NoiseTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *FilePath);

}

// Interpolation Weight
float ANoise::Fade(float t)
{
	return ((6 * t - 15) * t + 10) * t * t * t;
}

// Interpolation
float ANoise::Lerp(float t, float a1, float a2)
{
	return a1 + t * (a2 - a1);
}



