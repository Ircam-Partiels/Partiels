# Outils MCP de Partiels (Neuralyzer)

Liste des 29 outils déclarés dans [BinaryData/Neuralyzer/ToolsList.json](../Neuralyzer/ToolsList.json), dans l'ordre du fichier.

| Outil | Description |
| --- | --- |
| `search_docs` | Recherche sémantique dans la documentation et renvoie les documents pertinents triés par pertinence. |
| `load_docs` | Charge le contenu de documents de la documentation à partir de leurs identifiants. |
| `get_vamp_plugin_descriptions` | Renvoie la description complète de plugins Vamp à partir de leurs clés. |
| `get_document_state` | Renvoie l'état du document : groupes et pistes dans l'ordre d'affichage, zoom temporel, sélection temporelle, éléments sélectionnés et fichiers audio (toutes les propriétés par défaut). |
| `set_document_audio_file_layout` | Modifie la configuration des fichiers audio du document et l'assignation de leurs canaux. |
| `get_audio_file_info` | Renvoie les informations de fichiers audio (format, fréquence d'échantillonnage, durée, canaux, métadonnées…). |
| `get_track_properties` | Renvoie les propriétés demandées de pistes (nom, type, description, paramètres, graphismes, entrées, visibilité, seuils, zoom vertical, plage de valeurs, pistes liées). |
| `set_track_properties` | Modifie les propriétés de pistes (nom, visibilité, seuils, zoom vertical, plage de valeurs). |
| `get_group_properties` | Renvoie les propriétés demandées de groupes (nom, disposition des pistes, zoom vertical). |
| `set_group_properties` | Modifie les propriétés de groupes (nom, disposition des pistes, zoom vertical). |
| `read_files` | Lit des fichiers (texte analysé, ou image/audio injectés dans le contexte du modèle si possible). |
| `elicit` | Pose une question à l'utilisateur et attend sa réponse (précision, choix ou confirmation). |
| `set_document_time_zoom` | Modifie la plage temporelle visible (zoom temporel). |
| `get_track_results_raw` | Exporte les résultats bruts de pistes au format CSV (à appeler après `get_track_results_summary`). |
| `save_track_results` | Enregistre les résultats de pistes dans des fichiers d'un dossier temporaire, au format choisi. |
| `get_track_results_summary` | Calcule des statistiques sur les résultats de pistes (nombre, min, max, médiane, moyenne…). |
| `set_track_results_raw` | Remplace les résultats de pistes de marqueurs ou de valeurs dans une plage temporelle. |
| `erase_track_results` | Efface les résultats de pistes dans des plages temporelles. |
| `get_track_compatible_inputs` | Renvoie les pistes d'entrée compatibles avec des pistes. |
| `set_track_graphics` | Modifie les propriétés graphiques de pistes (dont dégradé de couleurs sur plusieurs pistes). |
| `set_track_parameters` | Modifie les valeurs des paramètres de pistes. |
| `set_linked_track_count` | Modifie le nombre de pistes liées d'une sortie multipiste. |
| `set_track_inputs` | Modifie les connexions des pistes d'entrée de pistes. |
| `create_tracks_with_plugins` | Crée des pistes à partir de plugins Vamp (y compris les pistes liées des sorties multipistes). |
| `create_tracks_with_files` | Crée des pistes à partir de fichiers de résultats d'analyse. |
| `remove_tracks` | Supprime des pistes. |
| `move_tracks` | Déplace des pistes dans leur groupe ou vers un autre groupe. |
| `create_groups` | Crée de nouveaux groupes. |
| `remove_groups` | Supprime des groupes avec leurs pistes. |
