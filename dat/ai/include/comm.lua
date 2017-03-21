function comm_bribe ()
   if mem.bribe_no then
      tk.msg("Bribe Pilot", mem.bribe_no)
      return
   end

   if not mem.bribe then
      tk.msg("Bribe Pilot", "\"Money won't save your hide now!\"")
      return
   end

   local prompt = mem.bribe_prompt
   if not prompt then
      prompt =  string.format("\"I'm gonna need at least %d credits to not leave you as a hunk of floating debris.\"\n\nPay %d credits?\"", mem.bribe, mem.bribe)
   end

   if not tk.yesno("Bribe Pilot", prompt) then
      tk.msg("Bribe Pilot", "You decide not to pay.")
      return
   end

   if player.credits() < mem.bribe then
      tk.msg("Bribe Pilot", "You don't have enough credits for the bribery.")
      return
   end

   player.pay(-mem.bribe)

   if mem.bribe_paid then
      tk.msg("Bribe Pilot", mem.bribe_paid)
   else
      tk.msg("Bribe Pilot", "\"Pleasure to do business with you.\"")
   end

   ai.pilot():setBribed()
   ai.pilot():setHostile(false)
end


function comm_refuel ()
   local player_stats = player.pilot():stats()
   local pilot_stats = ai.pilot():stats()
   local pilot_flags = ai.pilot():flags()

   if mem.refuel_no then
      tk.msg("Bribe Pilot", mem.refuel_no)
      return
   end

   if player_stats.fuel >= player_stats.fuel_max then
      tk.msg("Request Fuel", "Your fuel deposits are already full.")
      return
   end

   if pilot_stats.fuel < 200 then
      tk.msg("Request Fuel", "\"Sorry, I don't have enough fuel to spare at the moment.\"")
      return
   end

   if not mem.refuel or not mem.refuel_msg or pilot_flags.manualcontrol then
      tk.msg("Request Fuel", "\"Sorry, I'm busy now.\"")
      return
   end

   if pilot_flags.refueling then
      dialogue_msg( "Request Fuel", "Pilot is already refueling you." );
      return
   end

   if mem.refuel > 0 then
      if not tk.yesno("Request Fuel", string.format("%s\n\nPay %d credits?", mem.refuel_msg, mem.refuel)) then
         tk.msg("Request Fuel", "You decide not to pay.")
         return
      end
   else
      tk.msg("Request Fuel", mem.refuel_msg);
   end

   if mem.refuel > player.credits() then
      tk.msg("Request Fuel", string.format("You need %d more credits!", player.credits() - mem.refuel))
      return
   end

   player.pay(-mem.refuel)
   ai.pilot():refuel()

   if (mem.refuel > 0) then
      tk.msg("Request Fuel", "\"On my way.\"")
   end
end


function comm_handlers ()
   if mem.comm_no then
      player.msg(mem.comm_no)
      return
   end

   local handlers = {}
   if ai.pilot():hostile() and not ai.pilot():flags().bribed then
      handlers["Bribe"] = comm_bribe
   else
      handlers["Refuel"] = comm_refuel
   end
   return handlers
end
