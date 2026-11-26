# Подключение запуска TavernGamesVisual через диалог NPC

## Что уже сделано в исходниках

- В SKSE DLL зарегистрирована глобальная Papyrus-функция `TavernGamesNative.OpenGames()`.
- Эта функция напрямую подаёт команду открыть меню. Она не имитирует F8, поэтому не зависит от того, как Windows и DirectInput передают клавиши.
- Горячая клавиша F8 остаётся доступной как запасной способ открытия.

## Что нужно добавить в Creation Kit

В исходном архиве TavernGamesSE не было ESP/ESL с NPC-диалогами. Поэтому диалоговую запись нужно создать в Creation Kit и сохранить отдельным небольшим плагином. Готовый фрагмент для неё лежит в `TopicInfo_End_Fragment.txt`.

1. Скопируйте `Data/Scripts/Source/TavernGamesNative.psc` в `Data/Scripts/Source` вашей игры.
2. Откройте Creation Kit и загрузите `Skyrim.esm` как активный файл. Создайте новый файл, например `TavernGamesDialogue.esp`.
3. Создайте новый Quest с Editor ID `TG_TavernGamesDialogueQuest`, включите `Start Game Enabled`.
4. На вкладке Dialogue создайте ветку и Topic. Текст реплики игрока, например: `Хочешь сыграть в одну из тавернных игр?`
5. В Topic Info задайте ответ NPC, например: `Конечно. Выбирай игру и делай ставку.` В качестве Speaker выберите NPC, с которым должен появляться диалог. Для проверки можно выбрать Хульду в «Гарцующей кобыле».
6. В Topic Info откройте Scripts → End и вставьте содержимое `TopicInfo_End_Fragment.txt`. Нажмите Compile.
7. Убедитесь, что Creation Kit видит исходник `TavernGamesNative.psc`, и скомпилируйте его в `TavernGamesNative.pex`. Если CK не компилирует его автоматически, скомпилируйте через Papyrus Compiler из комплекта Creation Kit.
8. Сохраните `TavernGamesDialogue.esp` и включите его в менеджере модов.

После этого выберите реплику у указанного NPC: диалог закроется, и откроется лобби настольных игр.

## Важные требования

- Skyrim SE/AE 1.6.1170 и SKSE64 2.2.6.
- Address Library for SKSE Plugins, формат V5, соответствующий версии игры. Новый вариант DLL использует CommonLibSSE-NG для регистрации native Papyrus-функции.
- Для полноценного запуска из диалога нужны `TavernGamesVisual.dll`, `TavernGamesNative.pex` и созданный `TavernGamesDialogue.esp`. Сам `.psc` — исходник, игра не исполняет его напрямую.

## Если хотите добавить реплику другому NPC

В Creation Kit достаточно выбрать другого Speaker для Topic Info. Сам DLL менять не требуется.
