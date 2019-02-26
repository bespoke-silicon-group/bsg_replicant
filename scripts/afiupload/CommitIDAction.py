import os
import git
from argparse import Action, ArgumentTypeError

class CommitIDAction(Action):
        def __call__(self, parser, namespace, repo_at_commit_id, option_string=None):
                build_dir = getattr(namespace, "BuildDirectory")
                if(build_dir) is None:
                        raise KeyError(("BuildDirectory argument not "+ 
                                       "specified! BuildDirectory must be "+
                                       "specified before -{}").format(self.dest))

                repo, commit, path= self.validate(build_dir, repo_at_commit_id[0])
                repo_ids = getattr(namespace, self.dest)
                if(repo_ids is None):
                        repo_ids = {}
                repo_ids[repo] = {"commit": commit, "path":path}
                setattr(namespace, self.dest, repo_ids)
        
        def validate(self, build_dir, repo_at_commit_id):
                [name, commit] = repo_at_commit_id.split('@')
                short = os.path.join(build_dir, name)
                long = os.path.join(build_dir, name) + '_' + commit

                if os.path.isdir(short):
                        d = short
                elif os.path.isdir(long):
                        d = long
                else:
                        raise FileNotFoundError("Repo Directory not found!" +
                                                "Searched for {} and {}".format(short, long))

                r = git.Repo(d)
                sha = r.head.object.hexsha

                if(not sha.startswith(commit)):
                        raise ValueError("Commit ID argument {} does not match current commit {} of {}: ".format(commit[0:7], sha[0:7], d))
                return (name, commit, d)
