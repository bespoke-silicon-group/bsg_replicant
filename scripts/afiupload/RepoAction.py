import os
import git
from git.exc import CacheError
from argparse import Action, ArgumentTypeError

class RepoAction(Action):
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
        
        def validate(self, build_dir, repo):
                short = os.path.join(build_dir, repo)
                long = os.path.join(build_dir, repo)

                if os.path.isdir(short):
                        d = short
                elif os.path.isdir(long):
                        d = long
                else:
                        raise FileNotFoundError("Repo Directory not found!" +
                                                "Searched for {} and {}".format(short, long))

                r = git.Repo(d)
                i = r.index
                if len(i.diff(None)) != 0:
                        raise CacheError(f"Local {repo} repository differs " +
                                         "from remote. Have you committed " +
                                         "and pushed your changes?")

                sha = r.head.object.hexsha
                sha = sha[0:7]
                
                return (repo, sha, d)
