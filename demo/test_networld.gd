extends Node

var mesh_instance: MeshInstance3D
var vbox: VBoxContainer # On stocke la référence directe ici

func _ready() -> void:
	# 1. Création d'une représentation visuelle (un cube)
	mesh_instance = MeshInstance3D.new()
	mesh_instance.mesh = BoxMesh.new()
	add_child(mesh_instance)
	
	# 2. Ajout de la caméra avec rotation statique
	var camera = Camera3D.new()
	camera.position = Vector3(0, 7, 10)
	camera.rotation_degrees = Vector3(-35, 0, 0) # Remplace le look_at()
	add_child(camera)
	
	# 3. Interface Utilisateur de test
	vbox = VBoxContainer.new()
	add_child(vbox)
	
	var btn_host = Button.new()
	btn_host.text = "Démarrer en tant que HOST"
	btn_host.pressed.connect(_on_host_pressed)
	vbox.add_child(btn_host)
	
	var btn_client = Button.new()
	btn_client.text = "Démarrer en tant que CLIENT"
	btn_client.pressed.connect(_on_client_pressed)
	vbox.add_child(btn_client)

func _on_host_pressed() -> void:
	add_child(Networld)
	Networld.host(7070)
	
	Networld.bind_view_to_server_player(mesh_instance)
	print("[HOST] Démarré ! Le cube avance sur l'axe X grâce à Flecs (Velocity).")
	vbox.hide() # Utilisation de la référence sécurisée

func _on_client_pressed() -> void:
	add_child(Networld)
	Networld.join("127.0.0.1", 7070)

	Networld.bind_view_to_server_player(mesh_instance)
	print("[CLIENT] Connecté ! Le cube devrait suivre les paquets UDP du Host.")
	vbox.hide() # Utilisation de la référence sécurisée
