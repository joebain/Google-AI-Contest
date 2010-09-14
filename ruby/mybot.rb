#!/usr/bin/ruby

require './planetwars.rb'
require 'ostruct'

@outfile = File.new("output.log", 'w')
@centre = OpenStruct.new
@domestic_importance = 10.0
@foreign_importance = 20.0
@neutral_importance = 40.0
@min_ships = 10

@foreign_ships = 0
@domestic_ships = 0
@neutral_ships = 0
@total_ships = 0

@orders = 0

module Owners
	MINE = 1
	THEIRS = 2
	NEUTRAL = 0
end

def set_desires(planets)
	planets.each do |planet|
		planet.desire = (1.0 - (planet.num_ships / @total_ships)) * planet.growth_rate
		planet.distance = Math.sqrt( (planet.y - @centre.y)**2 + (planet.x - @centre.x)**2 )
		if planet.distance > 0 then
			planet.desire /= planet.distance
		end
		if planet.owner == Owners::MINE then
			planet.desire *= @domestic_importance
			planet.ships_needed = 0
		elsif planet.owner == Owners::NEUTRAL then
			planet.desire *= @neutral_importance
			planet.ships_needed = planet.num_ships
		else
			planet.desire *= @foreign_importance
			planet.ships_needed = planet.num_ships
		end

	end
end

def get_counts()
	@domestic_ships = 0
	@pw.my_planets.each do |planet|
		@domestic_ships += planet.num_ships
	end

	@foreign_ships = 0
	@pw.enemy_planets.each do |planet|
		@foreign_ships += planet.num_ships
	end

	@neutral_ships = 0
	@pw.neutral_planets.each do |planet|
		@neutral_ships += planet.num_ships
	end

	@total_ships = @domestic_ships + @foreign_ships + @neutral_ships
end

def find_centre(planets)
	running = OpenStruct.new
	running.x = 0
	running.y = 0
	planets.each do |planet|
		running.x += (planet.x * planet.num_ships)
		running.y += (planet.y * planet.num_ships)
	end
	running.x /= @domestic_ships
	running.y /= @domestic_ships
	@centre = running
	@outfile.puts "centre is #{@centre.x}, #{@centre.y}"
end

def order(source, target, ships)
	if (ships <= 0) then return end
	if (@pw.get_planet(source).num_ships < ships) then return end
	@outfile.puts "order! #{source} => #{target}, #{ships}"
	@orders += 1
	@pw.issue_order(source, target, ships)
end

def adjust_for_fleets()
	@pw.fleets.each do |fleet|
		planet = @pw.get_planet(fleet.destination_planet)
		if planet == nil then next end
		if fleet.owner == Owners::MINE then
			planet.ships_needed -= fleet.num_ships
		else
			planet.ships_needed += fleet.num_ships
			@outfile.puts "needs help: #{planet.to_s}"
		end
	end
end

def log_planets()
	@pw.planets.each do |planet|
		@outfile.puts planet.to_s
	end
end

def log_fleets()
	@pw.fleets.each do |fleet|
		@outfile.puts fleet.to_s
	end
end

def send_orders()

	target_i = 0
	source_i = @pw.my_planets.length-1
	while true do
		@outfile.puts "ordering"
		@outfile.puts("\n")

		@outfile.puts "find target"
		target_i -= 1
		target_planet = nil
		lim = @pw.planets.length
		loop do
			@outfile.puts "pork"
			target_i += 1
			if target_i >= lim then
				@outfile.puts "done"
				return	
			end
			@outfile.puts "beans"
			target_planet = @pw.planets[target_i]
			@outfile.puts "target: #{target_planet.to_s}"

			break unless target_planet.requested_ships <= 0
		end 
		@outfile.puts "find source"
		source_i += 1
		source_planet = nil
		loop do
			source_i -= 1
			if source_i < 0 then
				return
			end
			source_planet = @pw.my_planets[source_i]
			@outfile.puts "source: #{source_planet.to_s}"

			break unless source_planet.spare_ships <= 0
		end

		@outfile.puts "make orders"


		ships_to_send = 0
		if target_planet.owner != Owners::THEIRS
			if source_planet.spare_ships > target_planet.requested_ships then
				ships_to_send = target_planet.requested_ships + 1
			else
				ships_to_send = source_planet.spare_ships
			end
			order(
				source_planet.planet_id,
				target_planet.planet_id,
				ships_to_send )

				source_planet.remove_ships(ships_to_send)
				target_planet.ships_needed -= ships_to_send
		else
			needed_ships = target_planet.num_ships + target_planet.growth_rate * @pw.distance(source_planet.planet_id, target_planet.planet_id)
			if source_planet.spare_ships > needed_ships then
				ships_to_send = needed_ships + 1
			else
				ships_to_send = source_planet.spare_ships
			end
			order(
				source_planet.planet_id,
				target_planet.planet_id,
				ships_to_send )

				destruction_rate = target_planet.num_ships / needed_ships

				source_planet.remove_ships(ships_to_send)
				target_planet.ships_needed -= ships_to_send * destruction_rate
		end


	end
end

def do_turn()
	@orders = 0
	@outfile.puts("\n\nNEW TURN\n\n")
	get_counts()
	find_centre(@pw.my_planets)
	set_desires(@pw.planets)
	@pw.planets.sort! {|x,y| y.desire <=> x.desire}
	adjust_for_fleets()
	@outfile.puts "PLANETS:"
	log_planets()
	#@outfile.puts "FLEETS:"
	#log_fleets()
	@outfile.puts("\n")
	send_orders()
	@outfile.puts "finish turn, #{@orders} orders"
	@pw.finish_turn()
	@outfile.puts "sent orders"
end

map_data = []
loop do
	@outfile.puts "getting args"
	args = gets
	#if gets == nil then next end
	@outfile.puts args
	current_line = args.strip
	if current_line.length >= 2 and current_line[0..1] == "go"
		@outfile.puts "making planetwars"
		@pw = PlanetWars.new(map_data.join("\n"))
		do_turn()
		@pw.finish_turn
		map_data = []
	else
		map_data << current_line
	end
end
