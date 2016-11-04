-- Bacta code

local BACTA_SMALL_AMOUNT = 10
local BACTA_MEDIUM_AMOUNT = 25
local BACTA_LARGE_AMOUNT = 50

function GiveBactaSmall(ply, quantity)
	if ply.Health + BACTA_SMALL_AMOUNT > ply.MaxHealth then
		ply.Health = ply.MaxHealth
	else
		ply.Health = ply.Health + BACTA_SMALL_AMOUNT
	end

	ply.Entity:PlaySound(1, "sound/items/use_bacta.wav")
end

function GiveBactaMedium(ply, quantity)
	if ply.Health + BACTA_MEDIUM_AMOUNT > ply.MaxHealth then
		ply.Health = ply.MaxHealth
	else
		ply.Health = ply.Health + BACTA_MEDIUM_AMOUNT
	end

	ply.Entity:PlaySound(1, "sound/items/use_bacta.wav")
end

function GiveBactaLarge(ply, quantity)
	if ply.Health + BACTA_LARGE_AMOUNT > ply.MaxHealth then
		ply.Health = ply.MaxHealth
	else
		ply.Health = ply.Health + BACTA_LARGE_AMOUNT
	end

	ply.Entity:PlaySound(1, "sound/items/use_bacta.wav")
end

function Item_Bacta_Functions()
	items.AddConsumableFunction("item_bacta_small", GiveBactaSmall)
	items.AddConsumableFunction("item_bacta_medium", GiveBactaMedium)
	items.AddConsumableFunction("item_bacta_large", GiveBactaLarge)
end