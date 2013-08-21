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

local function AdmRank_GetRank(ply)
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

		jObjectItem = json.GetObjectItem( jObject, "can-rank-inspect" )
		rank["can-rank-inspect"] = json.ToBooleanOpt( jObjectItem, 1 )

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
		json.WriteBoolean( "can-rank-inspect", sortedrank["can-rank-inspect"] )
		json.WriteBoolean( "can-alter-rank", sortedrank["can-alter-rank"] )
		json.WriteBoolean( "can-alter-password", sortedrank["can-alter-password"] )
		json.WriteBoolean( "can-status", sortedrank["can-status"] )
		json.WriteBoolean( "can-say", sortedrank["can-say"] )
		json.WriteBoolean( "can-tell", sortedrank["can-tell"] )
		json.WriteBoolean( "can-speak", sortedrank["can-speak"] )
		json.WriteBoolean( "can-puppet", sortedrank["can-puppet"] )
		
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
	local k,v
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

local function AdminHelp_ListPowers( rank )
	-- This code is used in multiple places, I figured it would be wise to put this into a function
	if rank == nil then
		return
	end	

	if rank["can-changedetails"] then
		printnn("^3admchangedetails, ")
	end

	if rank["can-addaccounts"] then
		printnn("^6admnewaccount, ")
	end

	if rank["can-deleteaccounts"] then
		printnn("^6admdeleteaccount, ")
	end

	if rank["can-list-online"] then
		printnn("^2admlist online, ")
	end

	if rank["can-list-admins"] then
		printnn("^2admlist admins, ")
	end

	if rank["can-list-ranks"] then
		printnn("^2admlist ranks, ")
	end

	if rank["can-list-powers"] then
		printnn("^2admlist powers, ")
	end

	if rank["can-rank-inspect"] then
		printnn("^2admrank inspect, ")
	end

	if rank["can-alter-rank"] then
		printnn("^6admalter rank, ")
	end

	if rank["can-alter-password"] then
		printnn("^6admalter password, ")
	end

	if rank["can-status"] then
		printnn("^2admstatus, ")
	end

	if rank["can-say"] then
		printnn("^5admsay, ")
	end

	if rank["can-tell"] then
		printnn("^5admtell, ")
	end

	if rank["can-speak"] then
		printnn("^5admspeak, ")
	end

	if rank["can-puppet"] then
		printnn("^8admpuppet, ")
	end
						
	print(" ")
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
					AdmReply(ply, "^4Results printed to console.")
					while players.GetByID(k) ~= nil do
						local plysel = players.GetByID(k)
						if plysel:IsValid() then
							if plysel.IsAdmin then
								local plyselname = plysel:GetName()
								local plyselaccountname = plysel:GetAdminAccount()
								local plyselaccount = admins[plyselaccountname]
								if plyselaccount ~= nil then
									printnn(plysel:GetName() .. " [" .. plyselaccountname .. "--" .. plyselaccount["rank"] .. "], " )
								end
							end
						end
						k = k + 1
					end
					print(" ")
				end
			elseif argv[1] == "admins" then
				if rank["can-list-admins"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					AdmReply(ply, "^4Results printed to console.")
					print("^2All admin accounts:")
					local k
					for k = 0, admins["numAdmins"]-1 do
						printnn(sortedadmins[k]["username"] .. " [" .. sortedadmins[k]["rank"] .. "], ")
					end
					print(" ")
				end
			elseif argv[1] == "ranks" then
				if rank["can-list-ranks"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					AdmReply(ply, "^4Results printed to console.")
					print("^2All ranks:")
					local k
					for k = 0, ranks["numRanks"]-1 do
						printnn(sortedranks[k]["name"] .. ", ")
					end
					print(" ")
				end
			elseif argv[1] == "powers" then
				if rank["can-list-powers"] ~= true then
					AdmReply(ply, "^1You do not have permission to perform this action.")
				else
					AdmReply(ply, "^4Results printed to console.")
					print("^2Your powers:")
					
					AdminHelp_ListPowers( rank )
				end
				print(" ")
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
						print("^5This rank (" .. argv[2] .. ") can...")
						
						AdminHelp_ListPowers( inspectedrank )
					end
				end
			else
				AdmReply(ply, "^3Unknown admrank mode, valid modes are: inspect")
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
			while players.GetByID(k) ~= nil do
				local plysel = players.GetByID(k)
				if plysel:IsValid() then
					local plyselname = plysel:GetName()
					printnn( k .. " - " .. plyselname )
					if plysel.IsAdmin then
						local plyselaccountname = plysel:GetAdminAccount()
						local plyselaccount = admins[plyselaccountname]
						printnn( " ^7(^2Logged in as ^4" .. plyselaccountname .. " ^5[Rank: " .. plyselaccount["rank"] .. "]^7)" )
					end
					print(" ")
				end
				k = k + 1
			end
			print(" ")
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
end

--[[ ------------------------------------------------
	Init
--------------------------------------------------]]

local function InitAdminSys()
	InitAdminAccounts()
	InitAdminCmds()
end

InitAdminSys()