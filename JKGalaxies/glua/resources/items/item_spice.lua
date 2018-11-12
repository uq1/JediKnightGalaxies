--Spice Code

local SPICE_SMALL_AMOUNT = 10
local SPICE_LARGE_AMOUNT = 25

function GiveSpiceSmall(ply, quantity)
	ply:Damage(ply:GetEntity(ply), ply:GetEntity(ply), ply:GetOrigin(ply), ply:GetOrigin(ply), SPICE_SMALL_AMOUNT, 0, 3)
	--if ply.Health - SPICE_SMALL_AMOUNT < 1 then
		--ply.Health = 0
	--else
		--ply.Health = ply.Health - SPICE_SMALL_AMOUNT
	--end

	ply.Entity:PlaySound(1, "sound/items/use_bacta.wav")
end

function GiveSpiceLarge(ply, quantity)
	if ply.Health - SPICE_LARGE_AMOUNT < 1 then
		ply.Health = 0
	else
		ply.Health = ply.Health - SPICE_LARGE_AMOUNT
	end

	ply.Entity:PlaySound(1, "sound/items/use_bacta.wav")
end


function Item_Spice_Functions()
	items.AddConsumableFunction("item_spice_small", GiveSpiceSmall)
	items.AddConsumableFunction("item_spice_large", GiveSpiceLarge)
end