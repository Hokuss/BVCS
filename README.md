# BVCS - Basic Version Control System

## An audacious attempt to dethrone Git. (On a serious note: a very simplified and readable basic version control system.)

![Gemini_Generated_Image_9l18fz9l18fz9l18](https://github.com/user-attachments/assets/23bce804-34c7-4ee2-896a-df0b3c816db7)

## 🚀 Welcome to BVCS!

Tired of the complex incantations, the arcane ceremonies, and the bewildering array of options that modern version control systems present? Do you yearn for a simpler time, when tracking changes felt... intuitive?

BVCS is born from that yearning. It's not designed to manage the Linux kernel, nor to handle thousands of concurrent developers. Instead, it's an educational project, a thought experiment, and a genuinely humble attempt to demonstrate the core principles of version control in the most readable, approachable, and *basic* way possible.

I believe that understanding the fundamentals doesn't require a deep dive into cryptographic hashes, DAGs, or rebase vs. merge philosophical debates. BVCS aims to be a stepping stone, a "hello world" for version control, proving that even a basic system can provide immense value.

**Can it dethrone Git?** Probably not today. But it might just dethrone your fear of version control concepts!

## ✨ Working Commands/Features

* It no longer requires commands and uses ImGui to do that for you.
* Text Editing is also added in a very basic form. Soon it will have more features as well. 
* LZ4 Compression: All stored version data (the differences) are automatically compressed using the fast LZ4 algorithm to ensure efficient storage and faster operations.
* * Multiple Branch Support: Enable multiple contributors to work concurrently on the project and then merge their changes, especially when different files are modified across branches.

## ✨ Future Features

* Optimizations: Enhance performance and storage efficiency as the project evolves.
* Community Input: Your comments and suggestions are welcome and will be considered for future development!

## 🛠️ How it (Conceptually) Works

BVCS stores versions by creating simple, version number directories within the `.bvcs/version_history` folder. Each commit just stores the diffs (entire file/folder structure  that has changed) between the last version and current state of the project at the time of snapshot.

* `.bvcs/`: The root BVCS directory.
    * `config`: Stores basic repository configuration (e.g., just the version number and current branch name for time being for the time being).
    * `staging_area/`: Stores copies of files that have been "added/modified" (staged).
    * `version_history/`: Stores the differences.
        * `<branch_a>/`: A single branch of commits. 
    * `current_commit`: A current version of the project as per last snapshot.

## 🚀 Getting Started (Conceptual Usage)

Compile the app using requirements provided (not added yet) and launch. 
Or just download the app and copy it to base of project directory (not added yet either). 
