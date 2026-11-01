# Format `.fsm` — version 1

Un fichier `.fsm` décrit une machine à états en JSON. Il est lu par le moteur
(`getStateMachine`) et écrit par l'éditeur. Exemple complet :
`assets/fsm/sword_combo.fsm`.

## Racine

```json
{
  "version": 1,
  "name": "SwordCombo",
  "parameters": [ ... ],
  "states": [ ... ],
  "entryState": "Idle",
  "transitions": [ ... ]
}
```

| Champ         | Type   | Obligatoire | Description |
|---------------|--------|-------------|-------------|
| `version`     | int    | oui         | Version du format. Le loader refuse une version plus grande que celle qu'il connaît. |
| `name`        | string | non         | Nom lisible, pour les logs et l'éditeur. Le fichier est identifié par son chemin, pas par ce nom. |
| `parameters`  | array  | non (`[]`)  | Variables de la machine, voir [Paramètres](#paramètres). |
| `states`      | array  | oui (≥ 1)   | Voir [États](#états). |
| `entryState`  | string | oui         | Nom de l'état dans lequel la machine démarre. |
| `transitions` | array  | non (`[]`)  | Voir [Transitions](#transitions). |

Tous les noms (paramètres, états) sont **sensibles à la casse**.

## Paramètres

Variables propres à chaque entité. Le jeu les écrit (`setTrigger`, `setFloat`...),
les conditions des transitions les lisent.

```json
{ "name": "speed", "type": "float", "default": 0.0 }
```

| Champ     | Type   | Obligatoire | Description |
|-----------|--------|-------------|-------------|
| `name`    | string | oui         | Unique parmi les paramètres. Ne commence pas par `$` (réservé). |
| `type`    | string | oui         | `bool`, `float`, `int` ou `trigger`. |
| `default` | selon `type` | non   | Valeur initiale. Défaut : `false`, `0.0`, `0`. Toujours `false` pour un trigger. |

**Trigger** : un bool qui se remet tout seul à `false`. Il passe à `true` avec
`setTrigger` et reste à `true` tant qu'aucune transition qui le lit n'a été
prise. Quand une transition est prise, seuls **les triggers lus par cette
transition** sont remis à `false`. Conséquence : un appui fait avant que la
transition soit possible est gardé en mémoire (input buffering).

Un `int` écrit `1` dans le JSON pour un paramètre `float` est accepté et converti.

## États

```json
{
  "name": "Attack1",
  "action": { "type": "PlayPose", "pose": "slash_right", "duration": 0.5 },
  "editor": { "x": 250, "y": 0 }
}
```

| Champ    | Type   | Obligatoire | Description |
|----------|--------|-------------|-------------|
| `name`   | string | oui         | Unique parmi les états. `Any` est interdit (réservé). |
| `action` | object | non         | Ce que fait l'état. Absent = l'état ne fait rien. |
| `editor` | object | non         | Données de l'éditeur, **ignorées par le moteur**. |

### `action`

- `type` (string, obligatoire) : nom sous lequel la classe C++ est enregistrée
  dans `ActionRegistry` (ex. `"PlayPose"`).
- Tous les autres champs sont les **réglages** de cette action. Ils
  remplissent les champs exposés par la classe via `FieldVisitor`. Valeurs
  autorisées : bool, nombre, string. Pas d'objet imbriqué ni de tableau en v1.
- Un réglage absent garde la valeur par défaut de la classe. Un réglage que
  la classe ne connaît pas donne un warning.

Les réglages sont fixes : ils ne changent pas pendant la partie (contrairement
aux paramètres).

### `editor`

`x`, `y` (float) : position du nœud dans le graphe de l'éditeur. L'éditeur
peut ajouter d'autres champs ici (couleur, commentaire...) sans changer la
version du format.

## Transitions

```json
{ "from": "Attack1", "to": "Attack2",
  "conditions": [
    { "param": "attackPressed" },
    { "param": "$stateTime", "op": "<=", "value": 0.5 }
  ] }
```

| Champ        | Type   | Obligatoire | Description |
|--------------|--------|-------------|-------------|
| `from`       | string | oui         | Nom d'un état, ou `"Any"` (depuis n'importe quel état). |
| `to`         | string | oui         | Nom d'un état. |
| `conditions` | array  | non (`[]`)  | Toutes doivent être vraies (**ET**). Vide = transition immédiate. |
| `allowSelf`  | bool   | non (`false`) | Seulement avec `from: "Any"` : autorise la transition quand l'état courant est déjà `to`. |

Pour exprimer un **OU**, écrire plusieurs transitions avec le même `from` / `to`.

### Conditions

```json
{ "param": "speed", "op": ">", "value": 0.1 }
```

| Champ   | Description |
|---------|-------------|
| `param` | Nom d'un paramètre, ou une valeur intégrée (ci-dessous). |
| `op`    | `==`, `!=`, `<`, `>`, `<=`, `>=`. |
| `value` | Valeur comparée, du type du paramètre. |

- Pour un `bool` ou un `trigger`, `op` et `value` peuvent être omis : cela
  équivaut à `"op": "==", "value": true`.
- `<`, `>`, `<=`, `>=` sont interdits sur un `bool` ou un `trigger`.
- `==` et `!=` sur un `float` sont déconseillés (imprécision).

Valeurs intégrées (préfixe `$`, en lecture seule) :

| Nom          | Type  | Description |
|--------------|-------|-------------|
| `$stateTime` | float | Secondes passées dans l'état courant. Remis à 0 à chaque changement d'état. |

Condition C++ (cas spéciaux impossibles à exprimer avec des paramètres) :

```json
{ "custom": "IsGrounded", "expected": true }
```

`custom` est le nom enregistré dans `ConditionRegistry`. `expected` vaut `true`
par défaut.

## Évaluation (à chaque frame)

1. Si la machine n'a pas démarré : entrer dans `entryState` (`onEnter`). Fin.
2. Tester les transitions `from: "Any"` dans l'ordre du tableau, en sautant
   celles dont `to` est l'état courant (sauf `allowSelf`).
3. Puis tester les transitions dont `from` est l'état courant, dans l'ordre
   du tableau.
4. **La première transition valide gagne**. L'ordre du tableau est donc la
   priorité : dans l'exemple, `Attack1 → Attack2` est écrite avant
   `Attack1 → Idle`.
5. Si une transition est prise : `onExit` de l'ancien état → triggers de la
   transition remis à `false` → `$stateTime = 0` → `onEnter` du nouvel état.
6. **Une seule transition par frame** : on ne réévalue pas après un changement.
7. `onUpdate` de l'état courant, puis `$stateTime += dt`.

## Erreurs

Le loader refuse le fichier et affiche **toutes** les erreurs, chacune avec le
fichier et l'élément fautif (ex. `sword.fsm: transitions[3]: état "Atack2" inconnu`) :

- JSON mal formé
- `version` absente ou supérieure à la version supportée
- `states` vide, `entryState` absent ou inconnu
- nom d'état ou de paramètre en double
- état nommé `Any`, paramètre commençant par `$`
- `type` de paramètre inconnu, `default` du mauvais type
- `from` / `to` inconnu
- condition : `param` inconnu, `op` inconnu, `op` interdit pour le type, `value` du mauvais type
- `allowSelf` sur une transition qui ne part pas de `Any`

Warnings seulement (le fichier se charge, pour que l'éditeur puisse l'ouvrir
même si le code n'existe pas encore) :

- `action.type` non enregistré → l'état ne fait rien
- réglage d'action inconnu de la classe
- condition `custom` non enregistrée → la condition est toujours fausse

## Historique

- **1** : version initiale.
