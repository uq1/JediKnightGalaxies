--[[ ------------------------------------------------
	JKGalaxies Lua Code
	Admin system
		
	written by eezstreet
--------------------------------------------------]]

--[[ ------------------------------------------------
	Accounts
--------------------------------------------------]]

admins = { }
ranks = { }
-- Never access sortedadmins/sortedranks directly, it's used for saving et al
sortedadmins = { }
sortedranks = { }

function AdmRank_GetRank(ply)
	if ply.IsAdmin then
		local account = ply:GetAdminAccount()
		return ranks[admins[account]["rank"]]
	end
	return nil
end

local function AdmRank_InitRanks( )
	-- Grab our file
	local jRoot = json.RegisterFile("server/ranks.json")
	
	if jRoot == nil then
		json.Clear()
		securitylog("Could not register ranklist.json")
		return
	end

	-- Load up the list of ranks
	local rankArray = json.GetObjectItem( jRoot, "ranks" )
	if rankArray == nil then
		-- Lol, does not exist.
		json.Clear()
		securitylog("ranks array in ranks.json nonexisting.")
		return
	end

	-- Loop through objects
	local i
	for i = 0, json.GetArraySize(rankArray)-1 do
		local jObject = json.GetArrayItem( rankArray, i )
		local rank = {}
		local jObjectItem

		jObjectItem = json.GetObjectItem( jObject, "name" )
		rank["name"] = json.ToString( jObjectItem )

		--
		-- INSERT COMMAND-BASED STUFF HERE
		--

		jObjectItem = json.GetObjectItem( jObject, "can-changedetails" )
		rank["can-changedetails"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-addaccounts" )
		rank["can-addaccounts"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-deleteaccounts" )
		rank["can-deleteaccounts"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-list-online" )
		rank["can-list-online"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-list-admins" )
		rank["can-list-admins"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-list-powers" )
		rank["can-list-powers"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-list-ranks" )
		rank["can-list-ranks"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-list-powers" )
		rank["can-list-online"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-list-permissions" )
		rank["can-list-permissions"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-rank-inspect" )
		rank["can-rank-inspect"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-rank-create" )
		rank["can-rank-create"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-rank-delete" )
		rank["can-rank-delete"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-rank-addpermission" )
		rank["can-rank-addpermission"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-rank-deletepermission" )
		rank["can-rank-deletepermission"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-alter-rank" )
		rank["can-alter-rank"] = json.ToBooleanOpt( jObjectItem, 0 )
	
		jObjectItem = json.GetObjectItem( jObject, "can-alter-password" )
		rank["can-alter-password"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-status" )
		rank["can-status"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-say" )
		rank["can-say"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-tell" )
		rank["can-tell"] = json.ToBooleanOpt( jObjectItem, 1 )
		
		jObjectItem = json.GetObjectItem( jObject, "can-speak" )
		rank["can-speak"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-puppet" )
		rank["can-puppet"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-place" )
		rank["can-place"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-delent" )
		rank["can-delent"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-entcount" )
		rank["can-entcount"] = json.ToBooleanOpt( jObjectItem, 1 )

		jObjectItem = json.GetObjectItem( jObject, "can-showspawnvars" )
		rank["can-showspawnvars"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "can-rotate" )
		rank["can-rotate"] = json.ToBooleanOpt( jObjectItem, 0 )

		jObjectItem = json.GetObjectItem( jObject, "use-cheats" )
		rank["use-cheats"] = json.ToBooleanOpt( jObjectItem, 1 )
	
		--
		-- END COMMAND-BASED STUFF
		--

		rank["sorted"] = i

		ranks[rank["name"]] = rank
		table.insert(sortedranks, i, rank)
	end

	ranks["numRanks"] = json.GetArraySize(rankArray)

	-- Make sure to clean up our mess.
	json.Clear()
end

local function AdmList_InitAdminList ( )
	-- Grab our file
	local jRoot = json.RegisterFile("server/adminlist.json")

	if jRoot == nil then
		json.Clear()
		securitylog("Could not register adminlist.json")
		return
	end

	-- Instead of using a global Lua variable, let's load up a list of users
	local adminArray = json.GetObjectItem( jRoot, "admins" )
	if adminArray == nil then
		-- Lol, does not exist.
		json.Clear()
		securitylog("admins array in adminlist.json nonexisting.")
		return
	end

	-- ok, so loop through the objects
	local i
	for i = 0, json.GetArraySize(adminArray)-1 do
		local jObject = json.GetArrayItem( adminArray, i )
		local admin = {}
		local jObjectItem

		jObjectItem = json.GetObjectItem( jObject, "username" )
		admin["username"] = json.ToString( jObjectItem )
		
		jObjectItem = json.GetObjectItem( jObject, "password" )
		admin["password"] = json.ToString( jObjectItem )

		jObjectItem = json.GetObjectItem( jObject, "rank" )
		admin["rank"] = json.ToString( jObjectItem )

		if ranks[admin["rank"]] == nil then
			securitylog("WARNING: admin account " .. admin["username"] .. " pointing to invalid rank " .. admin["rank"])
			print("^3WARNING: admin account " .. admin["username"] .. " pointing to invalid rank " .. admin["rank"])
		end

		admin["sorted"] = i

		admins[admin["username"]] = admin
		table.insert(sortedadmins, i, admin)
		
	end

	admins["numAdmins"] = json.GetArraySize(adminArray)

	-- Make sure to clean up our mess.
	json.Clear()
end

-- High-Risk (Red/^1): Kicking/Banning, mostly. These are dangerous and should only be given to high-ranking admins.
-- Harmless (Green/^2): Status, listing stuff, help, etc. These aren't harmful from a security standpoint.
-- Self-Exploitable (Yellow/^3): From a security standpoint, these are only harmful to the person who uses it. Changing details falls into this category.
-- Building Commands (Blue/^4): Building stuff. Built stuff can be destroyed by the users (later on, when we implement that, anyway), so no harm done.
-- Communication (Cyan/^5): Talking with clients, and amongst admins. Can be extremely annoying if spoofed.
-- Structural (Magenta/^6): Alters the structure of administration, such as dealing with ranks. Not necessarily needed on accounts, but they're there for convenience.
-- Annoying (Orange/^8): While not necessarily harmful, if used, they can be annoying. Stuff like slap, slay, etc falls into this category.

local function AdminHelp_ListPowers( rank, listpermissions, listall )
	-- This code is used in multiple places, I figured it would be wise to put this into a function
	if rank == nil then
		return
	end

	local printedtext = ""

	if rank["can-changedetails"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^3can-changedetails^7, "
		else
			printedtext = printedtext .. "^3admchangedetails^7, "
		end
	end

	if rank["can-addaccounts"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^6can-addaccounts^7, "
		else
			printedtext = printedtext .. "^6admnewaccount^7, "
		end
	end

	if rank["can-deleteaccounts"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^6can-deleteaccounts^7, "
		else
			printedtext = printedtext .. "^6admdeleteaccount^7, "
		end
	end

	if rank["can-list-online"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^2can-list-online^7, "
		else
			printedtext = printedtext .. "^2admlist online^7, "
		end
	end

	if rank["can-list-admins"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^2can-list-admins^7, "
		else
			printedtext = printedtext .. "^2admlist admins^7, "
		end
	end

	if rank["can-list-ranks"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^2can-list-ranks^7, "
		else
			printedtext = printedtext .. "^2admlist ranks^7, "
		end
	end

	if rank["can-list-powers"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^2can-list-powers^7, "
		else
			printedtext = printedtext .. "^2admlist powers^7, "
		end
	end

	if rank["can-list-permissions"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^2can-list-permissions^7, "
		else
			printedtext = printedtext .. "^2admlist permissions^7, "
		end
	end

	if rank["can-rank-inspect"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^2can-rank-inspect^7, "
		else
			printedtext = printedtext .. "^2admrank inspect^7, "
		end
	end

	if rank["can-rank-create"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^1can-rank-create^7, "
		else
			printedtext = printedtext .. "^1admrank create^7, "
		end
	end

	if rank["can-rank-delete"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^1can-rank-delete^7, "
		else
			printedtext = printedtext .. "^1admrank delete^7, "
		end
	end

	if rank["can-rank-addpermission"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^1can-rank-addpermission^7, "
		else
			printedtext = printedtext .. "^1admrank addpermission^7, "
		end
	end

	if rank["can-rank-deletepermission"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^1can-rank-deletepermission^7, "
		else
			printedtext = printedtext .. "^1admrank deletepermission^7, "
		end
	end

	if rank["can-alter-rank"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^6can-alter-rank^7, "
		else
			printedtext = printedtext .. "^6admalter rank^7, "
		end
	end

	if rank["can-alter-password"] or listall == true then
		if listpermissions == true then
			printedtect = printedtext .. "^6can-alter-password^7, "
		else
			printedtext = printedtext .. "^6admalter password^7, "
		end
	end

	if rank["can-status"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^2can-status^7, "
		else
			printedtext = printedtext .. "^2admstatus^7, "
		end
	end

	if rank["can-say"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^5can-say^7, "
		else
			printedtext = printedtext .. "^5admsay^7, "
		end
	end

	if rank["can-tell"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^5can-tell^7, "
		else
			printedtext = printedtext .. "^5admtell^7, "
		end
	end

	if rank["can-speak"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^5can-speak^7, "
		else
			printedtext = printedtext .. "^5admspeak^7, "
		end
	end

	if rank["can-puppet"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^8can-puppet^7, "
		else
			printedtext = printedtext .. "^8admpuppet^7, "
		end
	end

	if rank["can-place"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^4can-place^7, "
		else
			printedtext = printedtext .. "^4bPlace^7, "
		end
	end

	if rank["can-delent"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^4can-delent^7, "
		else
			printedtext = printedtext .. "^4bDelent^7, "
		end
	end

	if rank["can-showspawnvars"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^4can-showspawnvars^7, "
		else
			printedtext = printedtext .. "^4bShowSpawnVars^7, "
		end
	end

	if rank["can-entcount"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^4can-entcount^7, "
		else
			printedtext = printedtext .. "^4bEntCount^7, "
		end
	end

	if rank["can-rotate"] or listall == true then
		if listpermissions == true then
			printedtext = printedtext .. "^4can-rotate^7, "
		else
			printedtext = printedtext .. "^4bRotate^7, "
		end
	end

	if listall == true or rank["use-cheats"] == true and listpermissions == true then
		printedtext = printedtext .. "^8can-cheat "
	end
						
	return printedtext
end

local function RankPermission_IsValid( permissionname )
	if permissionname == "can-changedetails" then
		return true
	elseif permissionname == "can-addaccounts" then
		return true
	elseif permissionname == "can-deleteaccounts" then
		return true
	elseif permissionname == "can-list-online" then
		return true
	elseif permissionname == "can-list-admins" then
		return true
	elseif permissionname == "can-list-powers" then
		return true
	elseif permissionname == "can-list-ranks" then
		return true
	elseif permissionname == "can-list-permissions" then
		return true
	elseif permissionname == "can-rank-inspect" then
		return true
	elseif permissionname == "can-rank-create" then
		return true
	elseif permissionname == "can-rank-delete" then
		return true
	elseif permissionname == "can-rank-addpermission" then
		return true
	elseif permissionname == "can-rank-deletepermission" then
		return true
	elseif permissionname == "can-alter-rank" then
		return true
	elseif permissionname == "can-alter-password" then
		return true
	elseif permissionname == "can-status" then
		return true
	elseif permissionname == "can-say" then
		return true
	elseif permissionname == "can-tell" then
		return true
	elseif permissionname == "can-speak" then
		return true
	elseif permissionname == "can-puppet" then
		return true
	elseif permissionname == "can-place" then
		return true
	elseif permissionname == "can-delent" then
		return true
	elseif permissionname == "can-showspawnvars" then
		return true
	elseif permissionname == "can-entcount" then
		return true
	elseif permissionname == "can-rotate" then
		return true
	elseif permissionname == "use-cheats" then
		return true
	else
		return false
	end
end

local function RankList_SaveRanks( reason )
	-- Save the .json file
	json.RegisterStream( 3, 1 )

	-- Root object
	json.BeginObject( "0" )

	-- "ranks" array
	json.BeginArray( "ranks" )

	-- Okay, now to save all of the ranks...
	local k
	for k = 0, ranks["numRanks"]-1 do
		-- Array base object
		json.BeginObject( "0" )

		local sortedrank = sortedranks[k]

		json.WriteString( "name", sortedrank["name"] )

		json.WriteBoolean( "can-changedetails", sortedrank["can-changedetails"] )
		json.WriteBoolean( "can-addaccounts", sortedrank["can-addaccounts"] )
		json.WriteBoolean( "can-deleteaccounts", sortedrank["can-deleteaccounts"] )
		json.WriteBoolean( "can-list-online", sortedrank["can-list-online"] )
		json.WriteBoolean( "can-list-admins", sortedrank["can-list-admins"] )
		json.WriteBoolean( "can-list-ranks", sortedrank["can-list-ranks"] )
		json.WriteBoolean( "can-list-powers", sortedrank["can-list-powers"] )
		json.WriteBoolean( "can-list-permissions", sortedrank["can-list-permissions"] )
		json.WriteBoolean( "can-rank-inspect", sortedrank["can-rank-inspect"] )
		json.WriteBoolean( "can-rank-create", sortedrank["can-rank-create"] )
		json.WriteBoolean( "can-rank-delete", sortedrank["can-rank-delete"] )
		json.WriteBoolean( "can-rank-addpermission", sortedrank["can-rank-addpermission"] )
		json.WriteBoolean( "can-rank-deletepermission", sortedrank["can-rank-deletepermission"] )
		json.WriteBoolean( "can-alter-rank", sortedrank["can-alter-rank"] )
		json.WriteBoolean( "can-alter-password", sortedrank["can-alter-password"] )
		json.WriteBoolean( "can-status", sortedrank["can-status"] )
		json.WriteBoolean( "can-say", sortedrank["can-say"] )
		json.WriteBoolean( "can-tell", sortedrank["can-tell"] )
		json.WriteBoolean( "can-speak", sortedrank["can-speak"] )
		json.WriteBoolean( "can-puppet", sortedrank["can-puppet"] )
		json.WriteBoolean( "can-place", sortedrank["can-place"] )
		json.WriteBoolean( "can-delent", sortedrank["can-delent"] )
		json.WriteBoolean( "can-showspawnvars", sortedrank["can-showspawnvars"] )
		json.WriteBoolean( "can-entcount", sortedrank["can-entcount"] )
		json.WriteBoolean( "can-rotate", sortedrank["can-rotate"] )
		json.WriteBoolean( "use-cheats", sortedrank["use-cheats"] )
		
		-- Array base object
		json.EndObject( )
	end

	-- "ranks" array
	json.EndObject()

	-- Root object
	json.EndObject()

	-- Now save it all up
	json.FinishStream( "server/ranks.json" )

	-- Log it to the security log
	securitylog("Ranks saved. (Reason: " .. reason .. " )")
end

local function UpdateRank( rank, reason )
	ranks[rank["name"]] = rank
	if sortedranks[rank["sorted"]] ~= nil then
		sortedranks[rank["sorted"]] = account
	else
		-- Likely a new rank
		table.insert(sortedranks, ranks["numRanks"]-1, rank)
	end
	RankList_SaveRanks( reason )
end

local function RankList_AddRank( rank )
	ranks["numRanks"] = ranks["numRanks"] + 1
	table.insert(sortedranks, ranks["numRanks"]-1, rank)
	UpdateRank( rank, "Added new rank: " .. rank["name"] )
	
end

local function RankList_DeleteRank( rank )
	local rankname = rank["name"]
	table.remove(sortedranks, rank["sorted"])
	ranks[rank["name"]] = nil
	-- One sec, make sure we aren't deleting the last rank...
	if ranks["numRanks"] == 1 then
		print("^1ERROR: admin tried to delete last rank...")
		return
	end
	-- Okay, we need to change the sorted stuff now
	local k
	for k = 0, ranks["numRanks"]-1 do
		if sortedranks[k] ~= nil and ranks[sortedranks[k]] ~= nil then
			ranks[sortedranks[k]]["sorted"] = k
		end
	end

	ranks["numRanks"] = ranks["numRanks"] - 1
	RankList_SaveRanks( "Deleted rank: " .. rankname )
end

local function AdmList_SaveAccounts( reason )
	-- Save the .json file
	json.RegisterStream( 3, 1 )
	
	-- Root object
	json.BeginObject( "0" )

	-- "admins" array
	json.BeginArray( "admins" )

	-- Okay, now we're going to save all the accounts...
	local k
	for k = 0, admins["numAdmins"]-1 do

		-- Array base object
		json.BeginObject( "0" )

		local sortedaccount = sortedadmins[k]

		json.WriteString( "username", sortedaccount["username"] )
		json.WriteString( "password", sortedaccount["password"] )
		json.WriteString( "rank", sortedaccount["rank"] )

		-- Array base object
		json.EndObject( )
	end

	-- "admins" array
	json.EndObject( )

	-- Root object
	json.EndObject( )

	-- Now save it all up
	json.FinishStream( "server/adminlist.json" )

	-- Log it to the security log
	securitylog("Accounts saved. (Reason: " .. reason .. " )")
end

local function UpdateAccount( account, reason )
	admins[account["username"]] = account
	if sortedadmins[account["sorted"]] ~= nil then
		sortedadmins[account["sorted"]] = account
	else
		-- Likely a new account
		table.insert(sortedadmins, admins["numAdmins"]-1, account)
	end
	AdmList_SaveAccounts( reason )
end

local function AdmList_AddAccount( account )
	admins["numAdmins"] = admins["numAdmins"] + 1
	table.insert(sortedadmins, admins["numAdmins"]-1, account)
	UpdateAccount( account, "Added new account: " .. account["username"] .. " [Rank: " .. account["rank"] .. "]" )
end

local function AdmList_DeleteAccount( account )
	local username = account["username"]
	table.remove(sortedadmins, account["sorted"])
	admins[account["username"]] = nil
	-- Okay, we need to change the sorted stuff now
	local k
	for k = 0, admins["numAdmins"]-1 do
		if sortedadmins[k] ~= nil and admins[sortedadmins[k]] ~= nil then
			admins[sortedadmins[k]]["sorted"] = k
		end
	end

	admins["numAdmins"] = admins["numAdmins"] - 1
	AdmList_SaveAccounts( "Deleted account: " .. username )
end

local function InitAdminAccounts( )
	AdmRank_InitRanks()
	AdmList_InitAdminList()
end

--[[ ------------------------------------------------
	Commands
--------------------------------------------------]]

local function AdmReply(ply, message)
	ply:SendChat("^7Admin System: " .. message)
end

local function Admin_Login(ply, argc, argv)
	if argc ~= 3 then
		-- Invalid number of arguments
		AdmReply(ply, "^1Invalid arguments")
		return
	end
	if ply.IsAdmin then
		-- We are admin, so no need to re-log us
		AdmReply(ply, "^5You are already logged in")
		return
	end
	if admins[argv[1]] ~= nil then
		if admins[argv[1]]["password"] == argv[2] then
			ply.IsAdmin = true
			ply:SetAdminAccount(argv[1])
			AdmReply(ply, "^2Admin login successful")

			local account = admins[argv[1]]
			local rank = ranks[account["rank"]]

			if rank["use-cheats"] == true then
				-- We can use cheats. Alright, mang! Let's set things up so that the C++ code knows that we can...
				ply.CanUseCheats = true
			end
			return
		end
	end
	AdmReply(ply, "^1Bad username/password")
end

local function Admin_Logout(ply, argc, argv)
	local k,v
	if ply.IsAdmin then
		ply.IsAdmin = false
		ply:SetAdminAccount(" ")
		if ply.CanUseCheats then
			ply.CanUseCheats = false -- don't want us using cheats when we shouldn't be able to...
		end
		AdmReply(ply, "^5You are now logged out")
		return
	end
	chatcmds.Ignore()
end

local function Admin_Kick(ply, argc, argv)
	local k,v
	if ply.IsAdmin then
		if argc < 2 then
			AdmReply(ply, "^1Please specify a player to kick")
			return
		end
		local target =  players.GetByArg(argv[1])
		if not target then
			AdmReply(ply, "^1Invalid player specified")
			return
		end
		local reason
		if argc > 2 then
			reason = table.concat(argv," ",2, argc-1)
		else 
			reason = nil
		end
		AdmReply(ply, "^5Player " .. target:GetName() .. " ^5has been kicked")
		target:Kick(reason)
		return
	end
	chatcmds.Ignore()
end

local function Admin_ChangeDetails(ply, argc, argv)
	if ply.IsAdmin then
		local rank = AdmRank_GetRank(ply)
		if rank["can-changedetails"] == true then
			-- Change our account details then?
			if argc < 2 then
				AdmReply(ply, "Please specify which details you'd like to change: password")
			elseif argv[1] == "password" or argv[1] == "Password" then
				-- We want to change the password on this account.
				-- Syntax:
				-- /admchangedetails password <oldpass> <newpass>
				if argc ~= 4 then
					AdmReply(ply, "Syntax: /admchangedetails password <oldpass> <newpass>")
				else
					local accountname = ply:GetAdminAccount()
					local account = admins[accountname]
					if argv[2] ~= account["password"] then
						AdmReply(ply, "Your old password did not match correctly. Please try again.")
					else
						-- kk, change.
						account["password"] = argv[3]
						UpdateAccount( account, account["username"] .. " changed their password" )
						AdmReply(ply, "^4Password changed.")
					end
				end
			else
				AdmReply(ply, "Invalid details type specified. Valid details: password")
			end
		else
			AdmReply(ply, "^4You don't have permission to perform this action.")
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_Profile(ply, argc, argv)
	if ply.IsAdmin then
		local account = ply:GetAdminAccount()
		AdmReply(ply, "^4You are logged in as " .. account .. " [Rank: " .. admins[account]["rank"] .. "]")
		return
	end
	chatcmds.Ignore()
end

local function Admin_AddAccount(ply, argc, argv)
	if ply.IsAdmin then
		-- Ya they're an admin...but are they authorized to do so?
		local rank = AdmRank_GetRank(ply)
		if rank["can-addaccounts"] ~= true then
			AdmReply(ply, "^1You do not have permission to perform this action.")
		else
			-- Syntax:
			-- /admnewaccount <username> <password> <rank>
			if argc ~= 4 then
				AdmReply(ply, "^3Syntax: /admnewaccount <username> <password> <rank>")
			else
				if admins[argv[1]] ~= nil then
					AdmReply(ply, "^1Could not create account, an account by this name already exists.")
				else
					local newaccount = {}
					newaccount["username"] = argv[1]
					newaccount["password"] = argv[2]
					newaccount["rank"] = argv[3]
					if ranks[newaccount["rank"]] ~= nil then
						AdmList_AddAccount( newaccount )
						AdmReply(ply, "^4Account added.")
					else
						AdmReply(ply, "^3The account you added does not have a valid rank. Please add the rank and try again." )
					end
				end
			end
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function AdmHelp_LogoutAccount(account)
	local k = 0
	while players.GetByID(k) ~= nil do
		ply = players.GetByID(k)
		if ply.IsAdmin then
			if account["username"] == ply:GetAdminAccount() then
				-- soz you've been booted
				AdmReply(ply, "^6Your account has been deleted. You have been logged out.")
				ply.IsAdmin = false
				ply:SetAdminAccount(" ")
			end
		end

		k = k + 1
	end
end

local function Admin_DeleteAccount(ply, argc, argv)
	if ply.IsAdmin then
		local rank = AdmRank_GetRank(ply)
		if rank["can-deleteaccounts"] ~= true then
			AdmReply(ply, "^1You do not have permission to perform this action.")
		else
			-- Syntax:
			-- /admdeleteaccount <username>
			if argc ~= 2 then
				AdmReply(ply, "^3Syntax: /admdeleteaccount <username>")
			else
				local desiredaccount = admins[argv[1]]
				if desiredaccount ~= nil then
					local ouraccount = admins[ply:GetAdminAccount()]
					if ouraccount["username"] ~= argv[1] then
						AdmHelp_LogoutAccount(desiredaccount)
						AdmList_DeleteAccount(desiredaccount)
						AdmReply(ply, "^4Account deleted.")
					else
						AdmReply(ply, "^1You cannot delete your own account.")
					end
				else
					AdmReply(ply, "^1Cannot delete account, it does not exist.")
				end
			end
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_List(ply, argc, argv)
	if ply.IsAdmin then
		if argc < 2 then
			AdmReply(ply, "^3Syntax: /admlist <online/admins/ranks/powers>")
		else
			local rank = AdmRank_GetRank(ply)
			if argv[1] == "online" then
				if rank["can-list-online"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					local k = 0
					local printstring = ""
					AdmReply(ply, "^4Results printed to console.")
					while players.GetByID(k) ~= nil do
						local plysel = players.GetByID(k)
						if plysel:IsValid() then
							if plysel.IsAdmin then
								local plyselname = plysel:GetName()
								local plyselaccountname = plysel:GetAdminAccount()
								local plyselaccount = admins[plyselaccountname]
								if plyselaccount ~= nil then
									printstring = printstring .. plysel:GetName() .. " [" .. plyselaccountname .. "--" .. plyselaccount["rank"] .. "], "
								end
							end
						end
						k = k + 1
					end
					ply:SendPrint(printstring)
				end
			elseif argv[1] == "admins" then
				if rank["can-list-admins"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					AdmReply(ply, "^4Results printed to console.")
					ply:SendPrint("^2All admin accounts:")
					local k
					local printstring = ""
					for k = 0, admins["numAdmins"]-1 do
						printstring = printstring .. sortedadmins[k]["username"] .. " [" .. sortedadmins[k]["rank"] .. "], "
					end
					ply:SendPrint(printstring)
				end
			elseif argv[1] == "ranks" then
				if rank["can-list-ranks"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					AdmReply(ply, "^4Results printed to console.")
					ply:SendPrint("^2All ranks:")
					local k
					local printstring = ""
					for k = 0, ranks["numRanks"]-1 do
						printstring = printstring .. sortedranks[k]["name"] .. ", "
					end
					ply:SendPrint(printstring)
				end
			elseif argv[1] == "powers" then
				if rank["can-list-powers"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					AdmReply(ply, "^4Results printed to console.")
					ply:SendPrint("^2Your powers:")
					
					ply:SendPrint(AdminHelp_ListPowers( rank, false, false ))
				end
			elseif argv[1] == "permissions" then
				if rank["can-list-permissions"] ~= true then	
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					AdmReply(ply, "^4Results printed to console.")
					ply:SendPrint("^2All permissions:")

					ply:SendPrint(AdminHelp_ListPowers( rank, true, true ))
				end
			else
				AdmReply(ply, "^3Unknown admlist mode. Valid modes are online, admins, ranks, powers")
			end
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_Rank(ply, argc, argv)
	if ply.IsAdmin then
		if argc < 2 then
			AdmReply(ply, "^3Syntax: /admrank <inspect> [...args...]")
		else
			local rank = AdmRank_GetRank(ply)
			if argv[1] == "inspect" then
				if argc < 3 then
					AdmReply(ply, "^3Syntax: /admrank inspect <rank>")
				elseif rank["can-rank-inspect"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					local inspectedrank = ranks[argv[2]]
					if inspectedrank == nil then
						AdmReply(ply, "^1'" .. argv[2] .. "' is not a valid rank.")
					else
						AdmReply(ply, "^4Rank permissions printed to console.")
						ply:SendPrint("^5This rank (" .. argv[2] .. ") can...")
						
						ply:SendPrint(AdminHelp_ListPowers( inspectedrank, false, false ))

						ply:SendPrint("^5Rank permissions:")

						ply:SendPrint(AdminHelp_ListPowers( inspectedrank, true, false ))
					end
				end
			elseif argv[1] == "create" then
				if argc < 3 then
					AdmReply(ply, "^3Syntax: /admrank create <rank name>")
				elseif rank["can-rank-create"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					local blankrank = {}
					blankrank["name"] = argv[2]
					RankList_AddRank( blankrank )
					AdmReply(ply, "^4Rank successfully added.")
				end
			elseif argv[1] == "delete" then
				if argc < 3 then
					AdmReply(ply, "^3Syntax: /admrank delete <rank> [change admins using this rank to THIS rank]")
				elseif rank["can-rank-delete"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					local rank = ranks[argv[2]]
					if rank == nil then
						AdmReply("^1ERROR: Invalid rank specified.")
					end

					-- Loop through the admins and make sure none of us are using this rank...
					local k
					for k = 0, admins["numAdmins"]-1 do
						if sortedadmins[k]["rank"]["name"] == argv[2] then
							if argc < 4 then
								AdmReply(ply, "^3This rank is in use. You will need to specify a rank for people to use, since this rank is gone.")
								return
							end
							sortedadmins[k]["rank"] = ranks[argv[3]]
						end
					end

					-- K, guess it's okay to kill the rank, now...
					-- You will be missed, rank.
					RankList_DeleteRank( rank )
					AdmReply(ply, "^4Rank deleted.")
				end
			elseif argv[1] == "addpermission" then
				if argc < 4 then
					AdmReply(ply, "^3Syntax: /admrank addpermission <rank> <permission>")
				elseif rank["can-rank-addpermission"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					local rank = ranks[argv[2]]
					if rank == nil then
						AdmReply(ply, "^1Invalid rank specified.")
						return
					end

					local permissionname = argv[3]
					if RankPermission_IsValid( permissionname ) == false then
						AdmReply(ply, "^1Invalid permission specified.")
						return
					end

					rank[permissionname] = true
					UpdateRank( rank, ply:GetAdminAccount() .. " added permission '" .. permissionname .. "' to rank " .. rank["name"])
					AdmReply(ply, "^4Rank updated successfully.")
				end
			elseif argv[1] == "deletepermission" then
				if argc < 4 then
					AdmReply(ply, "^3Syntax: /admrank deletepermission <rank> <permission>")
				elseif rank["can-rank-deletepermission"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					local rank = ranks[argv[2]]
					if rank == nil then
						AdmReply(ply, "^1Invalid rank specified.")
						return
					end

					local permissionname = argv[3]
					if RankPermission_IsValid( permissionname ) == false then
						AdmReply(ply, "^1Invalid permission specified.")
						return
					end

					rank[permissionname] = false
					UpdateRank( rank, ply:GetAdminAccount() .. " deleted permission '" .. permissionname .. "' from rank " .. rank["name"])
					AdmReply(ply, "^4Rank updated successfully.")
				end
			else
				AdmReply(ply, "^3Unknown admrank mode, valid modes are: inspect, create, delete, addpermission, deletepermission")
			end
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_Alter(ply, argc, argv)
	if ply.IsAdmin then
		local accountname = ply:GetAdminAccount()
		local rank = AdmRank_GetRank(ply)
		if argc ~= 4 then
			AdmReply(ply, "^3Syntax: /admalter <username> <rank/password> <change to..>")
		else
			local alteraccount = admins[argv[1]]
			if alteraccount == nil then
				AdmReply(ply, "^1Invalid username for /admalter.")
			else
				if argv[2] == "rank" then
					if rank["can-alter-rank"] ~= true then
						AdmReply(ply, "^1You do not have permission to perform this action.")
					else
						local newrank = argv[3]
						if ranks[newrank] == nil then
							AdmReply(ply, "^1Invalid rank.")
						else
							AdmReply(ply, "^4Account altered.")
							alteraccount["rank"] = argv[3]
							UpdateAccount( alteraccount, accountname .. " altered " .. alteraccount["username"] .. "'s rank (changed to " .. argv[3] .. ")" )
						end
					end
				elseif argv[2] == "password" then
					if rank["can-alter-password"] ~= true then
						AdmReply(ply, "^1You do not have permission to perform this action.")
					else
						AdmReply(ply, "^4Account altered.")
						alteraccount["password"] = argv[3]
						UpdateAccount( alteraccount, accountname .. " altered " .. alteraccount["username"] .. "'s password" )
					end
				else
					AdmReply(ply, "^3Invalid alter type. Valid types are rank, password.")
				end
			end
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_Status(ply, argc, argv)
	if ply.IsAdmin then
		local rank = AdmRank_GetRank(ply)
		if rank["can-status"] ~= true then
			AdmReply(ply, "^1You do not have permission to perform this action.")
		else
			local k = 0
			AdmReply(ply, "^4Results printed to console.")
			ply:SendPrint("^2Status:")
			local printstring = ""
			while players.GetByID(k) ~= nil do
				local plysel = players.GetByID(k)
				if plysel:IsValid() then
					local plyselname = plysel:GetName()
					printstring = printstring .. k .. " - " .. plyselname
					if plysel.IsAdmin then
						local plyselaccountname = plysel:GetAdminAccount()
						local plyselaccount = admins[plyselaccountname]
						
						printstring = printstring .. " ^7(^2Logged in as ^4" .. plyselaccountname .. " ^5[Rank: " .. plyselaccount["rank"] .. "]^7)"
					end
				end
				k = k + 1
			end
			ply:SendPrint(printstring)
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_Say(ply, argc, argv)
	if ply.IsAdmin then
		local rank = AdmRank_GetRank(ply)
		if rank["can-say"] ~= true then
			AdmReply(ply, "^1You do not have permission to perform this action.")
		else
			local accountname = ply:GetAdminAccount()
			local message = table.concat(argv," ",1, argc-1)
			chatmsg( "^7[^5" .. accountname .. "^7] <" .. rank["name"] .. "> " .. message )
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_Tell(ply, argc, argv)
	if ply.IsAdmin then
		local rank = AdmRank_GetRank(ply)
		if rank["can-tell"] ~= true then
			AdmReply(ply, "^1You do not have permission to perform this action.")
		else
			if argc < 3 then
				AdmReply(ply, "^3Syntax: /admtell <player id/name> <message>")
			else
				local plytarg = players.GetByArg(argv[1])
				if plytarg == nil then
					AdmReply(ply, "^1Invalid player specified.")
					return
				end

				local account = admins[ply:GetAdminAccount()]
				local message = table.concat(argv," ",1, argc-1)

				plytarg:SendChat( "^5Admin " .. account["username"] .. " whispers: " .. message )

				if plytarg.lastadmtell == nil then
					plytarg:SendChat( "^8Reply to this message using /admreply <msg>" )
				end
				plytarg.lastadmtell = account["username"]
			end
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

-- NOT to be confused with AdmReply!!
local function Admin_Reply(ply, argc, argv)
	if argc < 2 then
		AdmReply(ply, "^3Syntax: /admreply <message>")
		return
	end

	if ply.lastadmtell == nil then
		ply:SendChat( "^1You have not been messaged by an admin in this play session." )
		return
	end

	local account = ply.lastadmtell
	local k = 0
	local message = table.concat(argv," ",1, argc-1)
	while players.GetByID(k) ~= nil do
		local plytarg = players.GetByID(k)

		if( account == plytarg:GetAdminAccount() ) then
			plytarg:SendChat( "^5" .. ply.Name .. " replies: " .. message )
		end

		k = k + 1
	end

	ply:SendChat( "^5You reply: " .. message )
end

local function Admin_Speak(ply, argc, argv)
	if ply.IsAdmin then
		local rank = AdmRank_GetRank(ply)
		if rank["can-speak"] ~= true then
			AdmReply(ply, "^1You do not have permission to perform this action.")
			return
		end

		if argc < 2 then
			AdmReply(ply, "^3Syntax: /admspeak <message>")
			return
		end

		local k = 0
		local message = table.concat(argv," ",1, argc-1)
		local ouraccount = admins[ply:GetAdminAccount()]

		while players.GetByID(k) ~= nil do
			local plytarg = players.GetByID(k)

			if plytarg.IsAdmin then
				plytarg:SendChat( "^7[^4Admins^7] ^5" .. ouraccount["username"] .. "^7: " .. message )
			end

			k = k + 1
		end
		
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function Admin_Puppet(ply, argc, argv)
	-- EVIL... we can mimmick the actions of other people.
	-- Not for regular use, mostly for test purposes but some admins might find this humorous..

	if ply.IsAdmin then
		local rank = AdmRank_GetRank(ply)
		if rank["can-puppet"] ~= true then
			AdmReply(ply, "^1You do not have permission to perform this action.")
			return
		end

		if argc < 3 then
			AdmReply(ply, "^3Syntax: /admpuppet <player> <channel> <message> -OR-")
			AdmReply(ply, "/admpuppet <player> <message>")
			return
		end

		local plytarg = players.GetByArg(argv[1])
		if plytarg == nil then
			AdmReply(ply, "^1Invalid player specified.")
			return
		end

		local channeltarg = argv[2]
		local targetchan = 0

		if channeltarg == "global" then
			targetchan = 1
		elseif channeltarg == "yell" then
			targetchan = 2
		elseif channeltarg == "emote" then
			targetchan = 3
		elseif channeltarg == "me" then
			targetchan = 3
		elseif channeltarg == "action" then
			targetchan = 3
		elseif channeltarg == "team" then
			targetchan = 4
		end
		
		local message
		if targetchan == 0 then
			message = table.concat(argv," ",2, argc-1)
		else
			message = table.concat(argv," ",3, argc-1)
		end

		if targetchan == 0 then
			plytarg:ExecuteChatCommand("say " .. message)
		elseif targetchan == 1 then
			plytarg:ExecuteChatCommand("sayglobal " .. message)
		elseif targetchan == 2 then
			plytarg:ExecuteChatCommand("sayyell " .. message)
		elseif targetchan == 3 then
			plytarg:ExecuteChatCommand("sayact " .. message)
		elseif targetchan == 4 then
			plytarg:ExecuteChatCommand("say_team " .. message)
		else
			AdmReply(ply, "^1ERROR: This wasn't supposed to happen...abort, abort!")
			return
		end
	else
		AdmReply(ply, "^1You are not logged in.")
	end
end

local function InitAdminCmds()
	chatcmds.Add("admlogin", Admin_Login)
	chatcmds.Add("admlogout", Admin_Logout)
	chatcmds.Add("admkick", Admin_Kick)
	chatcmds.Add("admchangedetails", Admin_ChangeDetails)
	chatcmds.Add("admprofile", Admin_Profile)
	chatcmds.Add("admnewaccount", Admin_AddAccount)
	chatcmds.Add("admdeleteaccount", Admin_DeleteAccount)
	chatcmds.Add("admlist", Admin_List)
	chatcmds.Add("admrank", Admin_Rank)
	chatcmds.Add("admalter", Admin_Alter)
	chatcmds.Add("admstatus", Admin_Status)
	chatcmds.Add("admsay", Admin_Say)
	chatcmds.Add("admtell", Admin_Tell)
	chatcmds.Add("admspeak", Admin_Speak)
	chatcmds.Add("admpuppet", Admin_Puppet)

	-- For replying to /admtells..this isn't an admin command, it can be used by any client
	chatcmds.Add("admreply", Admin_Reply)

	-- Add them to the console too.
	cmds.Add("admlogin", Admin_Login)
	cmds.Add("admlogout", Admin_Logout)
	cmds.Add("admkick", Admin_Kick)
	cmds.Add("admchangedetails", Admin_ChangeDetails)
	cmds.Add("admprofile", Admin_Profile)
	cmds.Add("admnewaccount", Admin_AddAccount)
	cmds.Add("admdeleteaccount", Admin_DeleteAccount)
	cmds.Add("admlist", Admin_List)
	cmds.Add("admrank", Admin_Rank)
	cmds.Add("admalter", Admin_Alter)
	cmds.Add("admstatus", Admin_Status)
	cmds.Add("admsay", Admin_Say)
	cmds.Add("admtell", Admin_Tell)
	cmds.Add("admspeak", Admin_Speak)
	cmds.Add("admpuppet", Admin_Puppet)

	-- For replying to /admtells..this isn't an admin command, it can be used by any client
	cmds.Add("admreply", Admin_Reply)
end

--[[ ------------------------------------------------
	Init
--------------------------------------------------]]

local function InitAdminSys()
	InitAdminAccounts()
	InitAdminCmds()
end

InitAdminSys()