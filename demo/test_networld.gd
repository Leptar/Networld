extends Node

var host_world: NetWorld
var client_world: NetWorld

func _ready() -> void:
	
	
	if OS.has_feature("server"):
		host_world = NetWorld.new()
		host_world.name = "HostWorld"
		add_child(host_world)
		host_world.host(7070)
		print("[GDScript] Serveur démarré sur le port 7070.")
	else :
		client_world = NetWorld.new()
		client_world.name = "ClientWorld"
		add_child(client_world)
		client_world.join("127.0.0.1", 7070)
		print("[GDScript] Client connecté à 127.0.0.1:7070.")
		# 2. On attache ce cube à l'entité du monde client
		# 1. Création d'une représentation visuelle (un cube)
		var mesh_instance = MeshInstance3D.new()
		mesh_instance.mesh = BoxMesh.new()
		add_child(mesh_instance)
		client_world.bind_view_to_server_player(mesh_instance)
	
	
	
