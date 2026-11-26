# Запуск TavernGamesVisual через диалог

В этой версии добавлена Papyrus-функция `TavernGamesNative.OpenGames()`, зарегистрированная внутри SKSE DLL. Вызов открывает игровое меню напрямую, а не через синтетическое нажатие F8. F8 остаётся резервным способом запуска.

См. `Integration/SETUP_RU.md`: там описано, как создать Topic Info в Creation Kit и вставить End-фрагмент.

**Важно:** в исходном TavernGamesSE.zip не было диалогового ESP/ESL, поэтому архив с исходниками не может сам по себе добавить новую реплику NPC в игру. Для этого нужно создать `TavernGamesDialogue.esp` в Creation Kit и скомпилировать `TavernGamesNative.pex`.
